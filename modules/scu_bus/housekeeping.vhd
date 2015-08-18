library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.gencores_pkg.all;
use work.wb_scu_reg_pkg.all;
use work.remote_update_pkg.all;

entity housekeeping is
  generic ( 
            Base_addr:  unsigned(15 downto 0)
          );
  port (
        clk_sys:            in std_logic;
        clk_20Mhz:          in std_logic;
        n_rst:              in std_logic;

        ADR_from_SCUB_LA:   in std_logic_vector(15 downto 0);
        Data_from_SCUB_LA:  in std_logic_vector(15 downto 0);
        Ext_Adr_Val:        in std_logic;
        Ext_Rd_active:      in std_logic;
        Ext_Wr_active:      in std_logic;
        user_rd_active:     out std_logic;
        Data_to_SCUB:       out std_logic_vector(15 downto 0);
        Dtack_to_SCUB:      out std_logic;
        
        owr_pwren_o:        out std_logic_vector(1 downto 0);
        owr_en_o:           out std_logic_vector(1 downto 0);
        owr_i:              in std_logic_vector(1 downto 0);
  
        debug_serial_o:     out std_logic;
        debug_serial_i:     in  std_logic 
  );
end entity;


architecture housekeeping_arch of housekeeping is

 constant c_xwb_owm : t_sdb_device := (
    abi_class     => x"0000", -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7", -- 8/16/32-bit port granularity
    sdb_component => (
    addr_first    => x"0000000000000000",
    addr_last     => x"00000000000000ff",
    product       => (
    vendor_id     => x"000000000000CE42", -- CERN
    device_id     => x"779c5443",
    version       => x"00000001",
    date          => x"20120603",
    name          => "WR-Periph-1Wire    ")));

  constant c_xwb_uart : t_sdb_device := (
    abi_class     => x"0000", -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7", -- 8/16/32-bit port granularity
    sdb_component => (
    addr_first    => x"0000000000000000",
    addr_last     => x"00000000000000ff",
    product       => (
    vendor_id     => x"000000000000CE42", -- CERN
    device_id     => x"e2d13d04",
    version       => x"00000001",
    date          => x"20120603",
    name          => "WR-Periph-UART     ")));

  signal lm32_interrupt:   std_logic_vector(31 downto 0);
  signal lm32_rstn:        std_logic;

  -- Top crossbar layout
  constant c_slaves     : natural := 5;
  constant c_masters    : natural := 2;
  constant c_dpram_size : natural := 32768; -- in 32-bit words (64KB)
  constant c_layout     : t_sdb_record_array(c_slaves-1 downto 0) :=
   (0 => f_sdb_embed_device(f_xwb_dpram(c_dpram_size),  x"00000000"),
    1 => f_sdb_embed_device(c_xwb_owm,                  x"00100600"),
    2 => f_sdb_embed_device(c_xwb_uart,                 x"00100700"),
    3 => f_sdb_embed_device(c_xwb_scu_reg,              x"00100800"),
    4 => f_sdb_embed_device(c_wb_rem_upd_sdb,           x"00100900"));
  constant c_sdb_address : t_wishbone_address := x"00100000";

  signal cbar_slave_i : t_wishbone_slave_in_array (c_masters-1 downto 0);
  signal cbar_slave_o : t_wishbone_slave_out_array(c_masters-1 downto 0);
  signal cbar_master_i : t_wishbone_master_in_array(c_slaves-1 downto 0);
  signal cbar_master_o : t_wishbone_master_out_array(c_slaves-1 downto 0);
  
  signal slave_i : t_wishbone_slave_in;
  signal slave_o : t_wishbone_slave_out;

begin

  -- The top-most Wishbone B.4 crossbar
  interconnect : xwb_sdb_crossbar
   generic map(
     g_num_masters => c_masters,
     g_num_slaves => c_slaves,
     g_registered => true,
     g_wraparound => false, -- Should be true for nested buses
     g_layout => c_layout,
     g_sdb_addr => c_sdb_address)
   port map(
     clk_sys_i => clk_sys,
     rst_n_i => n_rst,
     -- Master connections (INTERCON is a slave)
     slave_i => cbar_slave_i,
     slave_o => cbar_slave_o,
     -- Slave connections (INTERCON is a master)
     master_i => cbar_master_i,
     master_o => cbar_master_o);

  -- The LM32 is master 0+1
  LM32 : xwb_lm32
    generic map(
      g_profile => "medium_icache_debug") -- Including JTAG and I-cache (no divide)
    port map(
      clk_sys_i => clk_sys,
      rst_n_i => n_rst,
      irq_i => lm32_interrupt,
      dwb_o => cbar_slave_i(0), -- Data bus
      dwb_i => cbar_slave_o(0),
      iwb_o => cbar_slave_i(1), -- Instruction bus
      iwb_i => cbar_slave_o(1));
  -- The other 31 interrupt pins are unconnected
  lm32_interrupt(31 downto 1) <= (others => '0');

  -- WB Slave 0 is the RAM
  ram : xwb_dpram
    generic map(
      g_size => c_dpram_size,
      g_slave1_interface_mode => PIPELINED,
      g_slave2_interface_mode => PIPELINED,
      g_slave1_granularity => BYTE,
      g_slave2_granularity => WORD,
      g_init_file => "housekeeping.mif")
    port map(
      clk_sys_i => clk_sys,
      rst_n_i => n_rst,
      -- First port connected to the crossbar
      slave1_i => cbar_master_o(0),
      slave1_o => cbar_master_i(0),
      -- Second port disconnected
      slave2_i => cc_dummy_slave_in, -- CYC always low
      slave2_o => open);
  
  --------------------------------------
  -- 1-WIRE
  --------------------------------------
  ONEWIRE : xwb_onewire_master
    generic map(
      g_interface_mode      => PIPELINED,
      g_address_granularity => BYTE,
      g_num_ports           => 2,
      g_ow_btp_normal       => "5.0",
      g_ow_btp_overdrive    => "1.0"
      )
    port map(
      clk_sys_i   => clk_sys,
      rst_n_i     => n_rst,

      -- Wishbone
      slave_i     => cbar_master_o(1),
      slave_o     => cbar_master_i(1),
      desc_o      => open,

      owr_pwren_o => owr_pwren_o,
      owr_en_o    => owr_en_o,
      owr_i       => owr_i
      );


  --------------------------------------
  -- UART
  --------------------------------------
  UART : xwb_simple_uart
    generic map(
      g_with_virtual_uart   => false,
      g_with_physical_uart  => true,
      g_interface_mode      => PIPELINED,
      g_address_granularity => BYTE
      )
    port map(
      clk_sys_i => clk_sys,
      rst_n_i   => n_rst,

      -- Wishbone
      slave_i => cbar_master_o(2),
      slave_o => cbar_master_i(2),
      desc_o  => open,

      uart_rxd_i => '0',
      uart_txd_o => debug_serial_o
      );

  -------------------------------------
  -- Interface to SCU Bus Slave
  -------------------------------------
  SCU_WB_Reg: wb_scu_reg
    generic map (
      Base_addr => Base_addr,
      register_cnt => 16 )
    port map (
      clk_sys_i => clk_sys,
      rst_n_i => n_rst,

      -- Wishbone
      slave_i => cbar_master_o(3),
      slave_o => cbar_master_i(3),

      Adr_from_SCUB_LA  => ADR_from_SCUB_LA,
      Data_from_SCUB_LA => Data_from_SCUB_LA,
      Ext_Adr_Val       => Ext_Adr_Val,
      Ext_Rd_active     => Ext_Rd_active,
      Ext_Wr_active     => Ext_Wr_active,
      user_rd_active    => user_rd_active,
      Data_to_SCUB      => Data_to_SCUB,
      Dtack_to_SCUB     => Dtack_to_SCUB);
      
  --------------------------------------------
  -- clock crossing from sys clk to clk_20Mhz
  --------------------------------------------
   cross_systo20 : xwb_clock_crossing
    port map(
      -- Slave control port
      slave_clk_i    => clk_sys,
      slave_rst_n_i  => n_rst,
      slave_i        => cbar_master_o(4),
      slave_o        => cbar_master_i(4),
      -- Master reader port
      master_clk_i   => clk_20Mhz,
      master_rst_n_i => n_rst,
      master_i       => slave_o,
      master_o       => slave_i);
  
  
  -----------------------------------------
  -- wb interface for altera remote update
  -----------------------------------------
  wb_aru: wb_remote_update
    port map (
      clk_sys_i => clk_20Mhz,
      rst_n_i   => n_rst,
      
      slave_i      =>  slave_i,
      slave_o      =>  slave_o);
    
  
      
      

end architecture;

