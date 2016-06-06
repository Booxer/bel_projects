-- libraries and packages
-- ieee
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- package declaration
package ghrd_5astfd5k3_pkg is
	component ghrd_5astfd5k3 is
		port (
			button_pio_external_connection_export : in    std_logic_vector(3 downto 0)  := (others => 'X'); -- export
			clk_clk                               : in    std_logic                     := 'X';             -- clk
			dipsw_pio_external_connection_export  : in    std_logic_vector(3 downto 0)  := (others => 'X'); -- export
			hps_0_f2h_cold_reset_req_reset_n      : in    std_logic                     := 'X';             -- reset_n
			hps_0_f2h_debug_reset_req_reset_n     : in    std_logic                     := 'X';             -- reset_n
			hps_0_f2h_stm_hw_events_stm_hwevents  : in    std_logic_vector(27 downto 0) := (others => 'X'); -- stm_hwevents
			hps_0_f2h_warm_reset_req_reset_n      : in    std_logic                     := 'X';             -- reset_n
			hps_0_h2f_reset_reset_n               : out   std_logic;                                        -- reset_n
			hps_io_hps_io_emac1_inst_TX_CLK       : out   std_logic;                                        -- hps_io_emac1_inst_TX_CLK
			hps_io_hps_io_emac1_inst_TXD0         : out   std_logic;                                        -- hps_io_emac1_inst_TXD0
			hps_io_hps_io_emac1_inst_TXD1         : out   std_logic;                                        -- hps_io_emac1_inst_TXD1
			hps_io_hps_io_emac1_inst_TX_CTL       : out   std_logic;                                        -- hps_io_emac1_inst_TX_CTL
			hps_io_hps_io_emac1_inst_RXD0         : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD0
			hps_io_hps_io_emac1_inst_RXD1         : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD1
			hps_io_hps_io_emac1_inst_TXD2         : out   std_logic;                                        -- hps_io_emac1_inst_TXD2
			hps_io_hps_io_emac1_inst_TXD3         : out   std_logic;                                        -- hps_io_emac1_inst_TXD3
			hps_io_hps_io_emac1_inst_MDIO         : inout std_logic                     := 'X';             -- hps_io_emac1_inst_MDIO
			hps_io_hps_io_emac1_inst_MDC          : out   std_logic;                                        -- hps_io_emac1_inst_MDC
			hps_io_hps_io_emac1_inst_RX_CTL       : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RX_CTL
			hps_io_hps_io_emac1_inst_RX_CLK       : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RX_CLK
			hps_io_hps_io_emac1_inst_RXD2         : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD2
			hps_io_hps_io_emac1_inst_RXD3         : in    std_logic                     := 'X';             -- hps_io_emac1_inst_RXD3
			hps_io_hps_io_qspi_inst_IO0           : inout std_logic                     := 'X';             -- hps_io_qspi_inst_IO0
			hps_io_hps_io_qspi_inst_IO1           : inout std_logic                     := 'X';             -- hps_io_qspi_inst_IO1
			hps_io_hps_io_qspi_inst_IO2           : inout std_logic                     := 'X';             -- hps_io_qspi_inst_IO2
			hps_io_hps_io_qspi_inst_IO3           : inout std_logic                     := 'X';             -- hps_io_qspi_inst_IO3
			hps_io_hps_io_qspi_inst_SS0           : out   std_logic;                                        -- hps_io_qspi_inst_SS0
			hps_io_hps_io_qspi_inst_CLK           : out   std_logic;                                        -- hps_io_qspi_inst_CLK
			hps_io_hps_io_sdio_inst_CMD           : inout std_logic                     := 'X';             -- hps_io_sdio_inst_CMD
			hps_io_hps_io_sdio_inst_D0            : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D0
			hps_io_hps_io_sdio_inst_D1            : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D1
			hps_io_hps_io_sdio_inst_CLK           : out   std_logic;                                        -- hps_io_sdio_inst_CLK
			hps_io_hps_io_sdio_inst_D2            : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D2
			hps_io_hps_io_sdio_inst_D3            : inout std_logic                     := 'X';             -- hps_io_sdio_inst_D3
			hps_io_hps_io_usb1_inst_D0            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D0
			hps_io_hps_io_usb1_inst_D1            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D1
			hps_io_hps_io_usb1_inst_D2            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D2
			hps_io_hps_io_usb1_inst_D3            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D3
			hps_io_hps_io_usb1_inst_D4            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D4
			hps_io_hps_io_usb1_inst_D5            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D5
			hps_io_hps_io_usb1_inst_D6            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D6
			hps_io_hps_io_usb1_inst_D7            : inout std_logic                     := 'X';             -- hps_io_usb1_inst_D7
			hps_io_hps_io_usb1_inst_CLK           : in    std_logic                     := 'X';             -- hps_io_usb1_inst_CLK
			hps_io_hps_io_usb1_inst_STP           : out   std_logic;                                        -- hps_io_usb1_inst_STP
			hps_io_hps_io_usb1_inst_DIR           : in    std_logic                     := 'X';             -- hps_io_usb1_inst_DIR
			hps_io_hps_io_usb1_inst_NXT           : in    std_logic                     := 'X';             -- hps_io_usb1_inst_NXT
			hps_io_hps_io_uart0_inst_RX           : in    std_logic                     := 'X';             -- hps_io_uart0_inst_RX
			hps_io_hps_io_uart0_inst_TX           : out   std_logic;                                        -- hps_io_uart0_inst_TX
			hps_io_hps_io_uart1_inst_RX           : in    std_logic                     := 'X';             -- hps_io_uart1_inst_RX
			hps_io_hps_io_uart1_inst_TX           : out   std_logic;                                        -- hps_io_uart1_inst_TX
			hps_io_hps_io_i2c0_inst_SDA           : inout std_logic                     := 'X';             -- hps_io_i2c0_inst_SDA
			hps_io_hps_io_i2c0_inst_SCL           : inout std_logic                     := 'X';             -- hps_io_i2c0_inst_SCL
			hps_io_hps_io_trace_inst_CLK          : out   std_logic;                                        -- hps_io_trace_inst_CLK
			hps_io_hps_io_trace_inst_D0           : out   std_logic;                                        -- hps_io_trace_inst_D0
			hps_io_hps_io_trace_inst_D1           : out   std_logic;                                        -- hps_io_trace_inst_D1
			hps_io_hps_io_trace_inst_D2           : out   std_logic;                                        -- hps_io_trace_inst_D2
			hps_io_hps_io_trace_inst_D3           : out   std_logic;                                        -- hps_io_trace_inst_D3
			hps_io_hps_io_trace_inst_D4           : out   std_logic;                                        -- hps_io_trace_inst_D4
			hps_io_hps_io_trace_inst_D5           : out   std_logic;                                        -- hps_io_trace_inst_D5
			hps_io_hps_io_trace_inst_D6           : out   std_logic;                                        -- hps_io_trace_inst_D6
			hps_io_hps_io_trace_inst_D7           : out   std_logic;                                        -- hps_io_trace_inst_D7
			hps_io_hps_io_gpio_inst_GPIO00        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO00
			hps_io_hps_io_gpio_inst_GPIO17        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO17
			hps_io_hps_io_gpio_inst_GPIO18        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO18
			hps_io_hps_io_gpio_inst_GPIO22        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO22
			hps_io_hps_io_gpio_inst_GPIO24        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO24
			hps_io_hps_io_gpio_inst_GPIO26        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO26
			hps_io_hps_io_gpio_inst_GPIO27        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO27
			hps_io_hps_io_gpio_inst_GPIO35        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO35
			hps_io_hps_io_gpio_inst_GPIO40        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO40
			hps_io_hps_io_gpio_inst_GPIO41        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO41
			hps_io_hps_io_gpio_inst_GPIO42        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO42
			hps_io_hps_io_gpio_inst_GPIO43        : inout std_logic                     := 'X';             -- hps_io_gpio_inst_GPIO43
			led_pio_external_connection_in_port   : in    std_logic_vector(3 downto 0)  := (others => 'X'); -- in_port
			led_pio_external_connection_out_port  : out   std_logic_vector(3 downto 0);                     -- out_port
			memory_mem_a                          : out   std_logic_vector(14 downto 0);                    -- mem_a
			memory_mem_ba                         : out   std_logic_vector(2 downto 0);                     -- mem_ba
			memory_mem_ck                         : out   std_logic;                                        -- mem_ck
			memory_mem_ck_n                       : out   std_logic;                                        -- mem_ck_n
			memory_mem_cke                        : out   std_logic;                                        -- mem_cke
			memory_mem_cs_n                       : out   std_logic;                                        -- mem_cs_n
			memory_mem_ras_n                      : out   std_logic;                                        -- mem_ras_n
			memory_mem_cas_n                      : out   std_logic;                                        -- mem_cas_n
			memory_mem_we_n                       : out   std_logic;                                        -- mem_we_n
			memory_mem_reset_n                    : out   std_logic;                                        -- mem_reset_n
			memory_mem_dq                         : inout std_logic_vector(39 downto 0) := (others => 'X'); -- mem_dq
			memory_mem_dqs                        : inout std_logic_vector(4 downto 0)  := (others => 'X'); -- mem_dqs
			memory_mem_dqs_n                      : inout std_logic_vector(4 downto 0)  := (others => 'X'); -- mem_dqs_n
			memory_mem_odt                        : out   std_logic;                                        -- mem_odt
			memory_mem_dm                         : out   std_logic_vector(4 downto 0);                     -- mem_dm
			memory_oct_rzqin                      : in    std_logic                     := 'X';             -- oct_rzqin
			reset_reset_n                         : in    std_logic                     := 'X'              -- reset_n
		);
	end component;
	
	component altera_wrapper_debounce is
		port (
			clk      : in std_logic;
			reset_n  : in std_logic;
			data_in  : in  std_logic_vector(3 downto 0);
			data_out : out std_logic_vector(3 downto 0)
		);
	end component;
end ghrd_5astfd5k3_pkg;
