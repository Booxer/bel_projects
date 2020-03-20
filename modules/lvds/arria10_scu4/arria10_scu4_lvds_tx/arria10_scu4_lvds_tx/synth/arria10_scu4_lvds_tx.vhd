-- arria10_scu4_lvds_tx.vhd

-- Generated using ACDS version 18.1 625

library IEEE;
library arria10_scu4_lvds_tx_altera_lvds_181;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use arria10_scu4_lvds_tx_altera_lvds_181.arria10_scu4_lvds_tx_pkg.all;

entity arria10_scu4_lvds_tx is
	port (
		ext_coreclock : in  std_logic                    := '0';             -- ext_coreclock.export
		ext_fclk      : in  std_logic                    := '0';             --      ext_fclk.export
		ext_loaden    : in  std_logic                    := '0';             --    ext_loaden.export
		tx_coreclock  : out std_logic;                                       --  tx_coreclock.export
		tx_in         : in  std_logic_vector(7 downto 0) := (others => '0'); --         tx_in.export
		tx_out        : out std_logic_vector(0 downto 0)                     --        tx_out.export
	);
end entity arria10_scu4_lvds_tx;

architecture rtl of arria10_scu4_lvds_tx is
begin

	lvds_0 : component arria10_scu4_lvds_tx_altera_lvds_181.arria10_scu4_lvds_tx_pkg.arria10_scu4_lvds_tx_altera_lvds_181_q6uosdy
		port map (
			tx_in         => tx_in,         --         tx_in.export
			tx_out        => tx_out,        --        tx_out.export
			tx_coreclock  => tx_coreclock,  --  tx_coreclock.export
			ext_fclk      => ext_fclk,      --      ext_fclk.export
			ext_loaden    => ext_loaden,    --    ext_loaden.export
			ext_coreclock => ext_coreclock  -- ext_coreclock.export
		);

end architecture rtl; -- of arria10_scu4_lvds_tx