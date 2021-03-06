--! @file        eca_tap_auto.vhd
--  DesignUnit   eca_tap_auto
--! @author      M. Kreider <m.kreider@gsi.de>
--! @date        27/09/2019
--! @version     0.0.1
--! @copyright   2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
--!

--! @brief AUTOGENERATED WISHBONE-SLAVE CORE FOR eca_tap.vhd
--!
--------------------------------------------------------------------------------
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3 of the License, or (at your option) any later version.
--!
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--! Lesser General Public License for more details.
--!
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library. If not, see <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------------------

-- ***********************************************************
-- ** WARNING - THIS IS AUTO-GENERATED CODE! DO NOT MODIFY! **
-- ***********************************************************
--
-- If you want to change the interface,
-- modify eca_tap.xml and re-run 'python wbgenplus.py eca_tap.xml' !

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.wishbone_pkg.all;
use work.wbgenplus_pkg.all;
use work.genram_pkg.all;
use work.eca_tap_auto_pkg.all;

entity eca_tap_auto is
Port(
  clk_sys_i     : std_logic;                            -- Clock input for sys domain
  rst_sys_n_i   : std_logic;                            -- Reset input (active low) for sys domain
  cnt_late_i    : in  std_logic_vector(32-1 downto 0);  -- Late Message Count
  cnt_late_V_i  : in  std_logic_vector(1-1 downto 0);   -- Valid flag - cnt_late
  cnt_msg_i     : in  std_logic_vector(64-1 downto 0);  -- Message Count
  cnt_msg_V_i   : in  std_logic_vector(1-1 downto 0);   -- Valid flag - cnt_msg
  diff_acc_i    : in  std_logic_vector(64-1 downto 0);  -- Accumulated differences (dl - ts)
  diff_acc_V_i  : in  std_logic_vector(1-1 downto 0);   -- Valid flag - diff_acc
  diff_max_i    : in  std_logic_vector(64-1 downto 0);  -- Maximum difference
  diff_max_V_i  : in  std_logic_vector(1-1 downto 0);   -- Valid flag - diff_max
  diff_min_i    : in  std_logic_vector(64-1 downto 0);  -- Minimum difference
  diff_min_V_i  : in  std_logic_vector(1-1 downto 0);   -- Valid flag - diff_min
  error_i       : in  std_logic_vector(1-1 downto 0);   -- Error control
  stall_i       : in  std_logic_vector(1-1 downto 0);   -- flow control
  capture_o     : out std_logic_vector(1-1 downto 0);   -- Enable/Disable Capture
  clear_o       : out std_logic_vector(4-1 downto 0);   -- b3: clear late count, b2: clear count/accu, b1: clear max, b0: clear min
  offset_late_o : out std_logic_vector(32-1 downto 0);  -- Offset on difference. Controls condition for Late Message Counter increment
  reset_o       : out std_logic_vector(1-1 downto 0);   -- Resets ECA-Tap
  
  ctrl_i        : in  t_wishbone_slave_in;
  ctrl_o        : out t_wishbone_slave_out

  
);
end eca_tap_auto;

architecture rtl of eca_tap_auto is

  signal s_pop, s_push    : std_logic;
  signal s_empty, s_full  : std_logic;
  signal r_e_wait, s_e_p  : std_logic;
  signal s_stall          : std_logic;
  signal s_valid,
         s_valid_ok,
         r_valid_check    : std_logic;
  signal r_ack            : std_logic;
  signal r_err            : std_logic;
  signal s_e, r_e, s_w    : std_logic;
  signal s_d              : std_logic_vector(32-1 downto 0);
  signal s_s              : std_logic_vector(4-1 downto 0);
  signal s_a              : std_logic_vector(4-1 downto 0);
  signal s_a_ext,
         r_a_ext0,
         r_a_ext1         : std_logic_vector(6-1 downto 0);
  signal r_error          : std_logic_vector(1-1 downto 0)  := std_logic_vector(to_unsigned(0, 1)); -- Error
  signal s_error_i        : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Error control
  signal s_stall_i        : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- flow control
  signal r_reset          : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Resets ECA-Tap
  signal r_clear          : std_logic_vector(4-1 downto 0)  := (others => '0');                     -- b3: clear late count, b2: clear count/accu, b1: clear max, b0: clear min
  signal r_capture        : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Enable/Disable Capture
  signal r_cnt_msg_V      : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - cnt_msg
  signal s_cnt_msg_V_i    : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - cnt_msg
  signal r_cnt_msg        : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Message Count
  signal s_cnt_msg_i      : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Message Count
  signal r_diff_acc_V     : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_acc
  signal s_diff_acc_V_i   : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_acc
  signal r_diff_acc       : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Accumulated differences (dl - ts)
  signal s_diff_acc_i     : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Accumulated differences (dl - ts)
  signal r_diff_min_V     : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_min
  signal s_diff_min_V_i   : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_min
  signal r_diff_min       : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Minimum difference
  signal s_diff_min_i     : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Minimum difference
  signal r_diff_max_V     : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_max
  signal s_diff_max_V_i   : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - diff_max
  signal r_diff_max       : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Maximum difference
  signal s_diff_max_i     : std_logic_vector(64-1 downto 0) := (others => '0');                     -- Maximum difference
  signal r_cnt_late_V     : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - cnt_late
  signal s_cnt_late_V_i   : std_logic_vector(1-1 downto 0)  := (others => '0');                     -- Valid flag - cnt_late
  signal r_cnt_late       : std_logic_vector(32-1 downto 0) := (others => '0');                     -- Late Message Count
  signal s_cnt_late_i     : std_logic_vector(32-1 downto 0) := (others => '0');                     -- Late Message Count
  signal r_offset_late    : std_logic_vector(32-1 downto 0) := (others => '0');                     -- Offset on difference. Controls condition for Late Message Counter increment


begin

  sp : wb_skidpad
  generic map(
    g_adrbits   => 4
  )
  Port map(
    clk_i        => clk_sys_i,
    rst_n_i      => rst_sys_n_i,
    push_i       => s_push,
    pop_i        => s_pop,
    full_o       => s_full,
    empty_o      => s_empty,
    adr_i        => ctrl_i.adr(5 downto 2),
    dat_i        => ctrl_i.dat,
    sel_i        => ctrl_i.sel,
    we_i         => ctrl_i.we,
    adr_o        => s_a,
    dat_o        => s_d,
    sel_o        => s_s,
    we_o         => s_w
  );

  validmux: with to_integer(unsigned(s_a_ext)) select
  s_valid <= 
  s_cnt_msg_V_i(0)  when c_cnt_msg_GET_1,   -- 
  s_cnt_msg_V_i(0)  when c_cnt_msg_GET_0,   -- 
  s_diff_acc_V_i(0) when c_diff_acc_GET_1,  -- 
  s_diff_acc_V_i(0) when c_diff_acc_GET_0,  -- 
  s_diff_min_V_i(0) when c_diff_min_GET_1,  -- 
  s_diff_min_V_i(0) when c_diff_min_GET_0,  -- 
  s_diff_max_V_i(0) when c_diff_max_GET_1,  -- 
  s_diff_max_V_i(0) when c_diff_max_GET_0,  -- 
  s_cnt_late_V_i(0) when c_cnt_late_GET,    -- 
  '1'               when others;
  
  s_valid_ok      <=  r_valid_check and s_valid;
  s_e_p           <=  r_e or r_e_wait;
  s_a_ext         <= s_a & "00";
  s_stall         <= s_full;
  s_push          <= ctrl_i.cyc and ctrl_i.stb and not s_stall;
  s_e             <= not (s_empty or s_e_p);
  s_pop           <= s_valid_ok;
  ctrl_o.stall    <= s_stall;
  
  s_error_i       <= error_i;
  s_stall_i       <= stall_i;
  reset_o         <= r_reset;
  clear_o         <= r_clear;
  capture_o       <= r_capture;
  s_cnt_msg_V_i   <= cnt_msg_V_i;
  s_cnt_msg_i     <= cnt_msg_i;
  s_diff_acc_V_i  <= diff_acc_V_i;
  s_diff_acc_i    <= diff_acc_i;
  s_diff_min_V_i  <= diff_min_V_i;
  s_diff_min_i    <= diff_min_i;
  s_diff_max_V_i  <= diff_max_V_i;
  s_diff_max_i    <= diff_max_i;
  s_cnt_late_V_i  <= cnt_late_V_i;
  s_cnt_late_i    <= cnt_late_i;
  offset_late_o   <= r_offset_late;
  
  ctrl : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_sys_n_i = '0') then
        r_e           <= '0';
        r_e_wait      <= '0';
        r_valid_check <= '0';
        r_error       <= std_logic_vector(to_unsigned(0, 1));
        r_reset       <= (others => '0');
        r_clear       <= (others => '0');
        r_capture     <= (others => '0');
        r_cnt_msg     <= (others => '0');
        r_diff_acc    <= (others => '0');
        r_diff_min    <= (others => '0');
        r_diff_max    <= (others => '0');
        r_cnt_late    <= (others => '0');
        r_offset_late <= (others => '0');
      else
        r_e           <= s_e;
        r_a_ext0      <= s_a_ext;
        r_a_ext1      <= r_a_ext0;
        r_e_wait      <= s_e_p and not s_valid_ok;
        r_valid_check <= (r_valid_check or (s_e_p and not stall_i(0))) and not s_valid_ok;
        r_ack         <= s_pop and not (error_i(0) or r_error(0));
        r_err         <= s_pop and     (error_i(0) or r_error(0));
        ctrl_o.ack    <= r_ack;
        ctrl_o.err    <= r_err;
        
        
        if stall_i = "0" then
          r_clear <= (others => '0');
          r_error <= (others => '0');
          r_reset <= (others => '0');
        end if;
        
        if s_cnt_late_V_i = "1" then r_cnt_late <= s_cnt_late_i; end if;  -- 
        if s_cnt_msg_V_i  = "1" then r_cnt_msg  <= s_cnt_msg_i; end if;   -- 
        if s_diff_acc_V_i = "1" then r_diff_acc <= s_diff_acc_i; end if;  -- 
        if s_diff_max_V_i = "1" then r_diff_max <= s_diff_max_i; end if;  -- 
        if s_diff_min_V_i = "1" then r_diff_min <= s_diff_min_i; end if;  -- 
        
        
        if(s_e = '1') then
          if(s_w = '1') then
            -- WISHBONE WRITE ACTIONS
            case to_integer(unsigned(s_a_ext)) is
              when c_reset_OWR      => r_reset        <= f_wb_wr(r_reset, s_d, s_s, "owr");       -- 
              when c_clear_OWR      => r_clear        <= f_wb_wr(r_clear, s_d, s_s, "owr");       -- 
              when c_capture_RW     => r_capture      <= f_wb_wr(r_capture, s_d, s_s, "owr");     -- 
              when c_offset_late_RW => r_offset_late  <= f_wb_wr(r_offset_late, s_d, s_s, "owr"); -- 
              when others           => r_error        <= "1";
            end case;
          else
            -- WISHBONE READ ACTIONS
            case to_integer(unsigned(s_a_ext)) is
              when c_capture_RW     => null;
              when c_cnt_msg_GET_1  => null;
              when c_cnt_msg_GET_0  => null;
              when c_diff_acc_GET_1 => null;
              when c_diff_acc_GET_0 => null;
              when c_diff_min_GET_1 => null;
              when c_diff_min_GET_0 => null;
              when c_diff_max_GET_1 => null;
              when c_diff_max_GET_0 => null;
              when c_cnt_late_GET   => null;
              when c_offset_late_RW => null;
              when others           => r_error <= "1";
            end case;
          end if; -- s_w
        end if; -- s_e
        
        case to_integer(unsigned(r_a_ext1)) is
          when c_capture_RW     => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_capture), ctrl_o.dat'length));                  -- 
          when c_cnt_msg_GET_1  => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_cnt_msg(63 downto 32)), ctrl_o.dat'length));    -- 
          when c_cnt_msg_GET_0  => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_cnt_msg(31 downto 0)), ctrl_o.dat'length));     -- 
          when c_diff_acc_GET_1 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_acc(63 downto 32)), ctrl_o.dat'length));   -- 
          when c_diff_acc_GET_0 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_acc(31 downto 0)), ctrl_o.dat'length));    -- 
          when c_diff_min_GET_1 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_min(63 downto 32)), ctrl_o.dat'length));   -- 
          when c_diff_min_GET_0 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_min(31 downto 0)), ctrl_o.dat'length));    -- 
          when c_diff_max_GET_1 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_max(63 downto 32)), ctrl_o.dat'length));   -- 
          when c_diff_max_GET_0 => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_diff_max(31 downto 0)), ctrl_o.dat'length));    -- 
          when c_cnt_late_GET   => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_cnt_late), ctrl_o.dat'length));                 -- 
          when c_offset_late_RW => ctrl_o.dat <= std_logic_vector(resize(unsigned(r_offset_late), ctrl_o.dat'length));              -- 
          when others           => ctrl_o.dat <= (others => 'X');
        end case;

        
      end if; -- rst
    end if; -- clk edge
  end process;

end rtl;
