--! @file        eca_tap_auto_pkg.vhd
--  DesignUnit   eca_tap_auto
--! @author      M. Kreider <m.kreider@gsi.de>
--! @date        27/09/2019
--! @version     0.0.1
--! @copyright   2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
--!

--! @brief AUTOGENERATED WISHBONE-SLAVE PACKAGE FOR eca_tap.vhd
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
package eca_tap_auto_pkg is

  constant c_reset_OWR      : natural := 16#00#;  -- wo,  1 b, Resets ECA-Tap
  constant c_clear_OWR      : natural := 16#04#;  -- wo,  4 b, b3: clear late count, b2: clear count/accu, b1: clear max, b0: clear min
  constant c_capture_RW     : natural := 16#08#;  -- rw,  1 b, Enable/Disable Capture
  constant c_cnt_msg_GET_1  : natural := 16#0c#;  -- ro, 32 b, Message Count
  constant c_cnt_msg_GET_0  : natural := 16#10#;  -- ro, 32 b, Message Count
  constant c_diff_acc_GET_1 : natural := 16#14#;  -- ro, 32 b, Accumulated differences (dl - ts)
  constant c_diff_acc_GET_0 : natural := 16#18#;  -- ro, 32 b, Accumulated differences (dl - ts)
  constant c_diff_min_GET_1 : natural := 16#1c#;  -- ro, 32 b, Minimum difference
  constant c_diff_min_GET_0 : natural := 16#20#;  -- ro, 32 b, Minimum difference
  constant c_diff_max_GET_1 : natural := 16#24#;  -- ro, 32 b, Maximum difference
  constant c_diff_max_GET_0 : natural := 16#28#;  -- ro, 32 b, Maximum difference
  constant c_cnt_late_GET   : natural := 16#2c#;  -- ro, 32 b, Late Message Count
  constant c_offset_late_RW : natural := 16#30#;  -- rw, 32 b, Offset on difference. Controls condition for Late Message Counter increment

  --| Component ----------------------- eca_tap_auto ------------------------------------------|
  component eca_tap_auto is
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
  end component;

  constant c_eca_tap_ctrl_sdb : t_sdb_device := (
  abi_class     => x"0000", -- undocumented device
  abi_ver_major => x"01",
  abi_ver_minor => x"00",
  wbd_endian    => c_sdb_endian_big,
  wbd_width     => x"7", -- 8/16/32-bit port granularity
  sdb_component => (
  addr_first    => x"0000000000000000",
  addr_last     => x"000000000000003f",
  product => (
  vendor_id     => x"0000000000000651",
  device_id     => x"0eca07a2",
  version       => x"00000001",
  date          => x"20190927",
  name          => "ECA-Tap            ")));

end eca_tap_auto_pkg;
package body eca_tap_auto_pkg is
end eca_tap_auto_pkg;
