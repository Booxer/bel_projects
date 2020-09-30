----------------------------------------------------------------------------------------------------- 
 -- Prototype matrix test configuration 				
--
--		3 x [5 electrical inputs and 1 electrical output]
--		2 x [5 optical inputs and 1 optical outputs]	  
---------------------------------------------------------------------------------------------------
 --		5 optical inputs and 1 optical outputs card			|	ID	65 01000001
 --		5 electrical inputs and 1 electrical output card	|	ID	66 01000010
 --		Intermediate backplane FG902.050					|	ID  67 01000011
-----------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------
-- Expected matrix configurations
----------------------------------------------------------------------------------------------------- 
--		6 electrical inputs									|	ID ( 74 --to be checked) 01001010
--		6 optical inputs									|	ID ( 75 --to be checked) 01001011
--		6 optical outputs									|	ID ( 76 --to be checked) 01001100
--		New Intermediate backplane							|	ID ( 77 --to be checked) 01001101

----------------------------------------------------------------------------------------------------- 
-- 4 new matrix configurations:
----------------------------------------------------------------------------------------------------- 
 -- STANDARD MATRIX 										
 --		9 x [6 electrical inputs] 
 --		3 x [6 optical outputs]
----------------------------------------------------------------------------------------------------- 
 --	MIXED INPUT MATRIX 										
 --		7 x [6 electrical inputs]
 --		2 x [6 optical inputs]  
 --		3 x [6 optical outputs]						
----------------------------------------------------------------------------------------------------- 
 -- OPTICAL MATRIX 											
 --		9 x [6 optical inputs] 
 --		1 x [6 optical outputs]
----------------------------------------------------------------------------------------------------- 

-----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity qud_trig_matrix is

PORT
(
clk : in std_logic;
  nReset: in std_logic;
  read_ID_ena : in std_logic;
  slave1_ID: in std_logic_vector(7 downto 0);
  slave2_ID: in std_logic_vector(7 downto 0);
  slave3_ID: in std_logic_vector(7 downto 0);
  slave4_ID: in std_logic_vector(7 downto 0);
  slave5_ID: in std_logic_vector(7 downto 0);
  slave6_ID: in std_logic_vector(7 downto 0);
  slave7_ID: in std_logic_vector(7 downto 0);
  slave8_ID: in std_logic_vector(7 downto 0);
  slave9_ID: in std_logic_vector(7 downto 0);
  slave10_ID: in std_logic_vector(7 downto 0);
  slave11_ID: in std_logic_vector(7 downto 0);
  slave12_ID: in std_logic_vector(7 downto 0);
  Trigger_matrix_Config:out  std_logic_vector(7 downto 0) -- maximum 5!= 120 theoretically possible configurations (01111000)
);
end qud_trig_matrix;


architecture qud_trig_matrix_arch of qud_trig_matrix is

type   t_reg_array         is array (1 to 12) of std_logic_vector(7 downto 0);
signal conf_reg:           t_reg_array; 
signal 	Matrix_conf: std_logic_vector(35 downto 0);
signal  IN_LEMO_prot_cnt: integer range 0 to 12; 
signal  IN_OPT_prot_cnt: integer range 0 to 12; 
signal  IN_LEMO_cnt: integer range 0 to 12; 
signal  IN_OPT_I_cnt: integer range 0 to 12; 
signal  IN_OPT_o_cnt: integer range 0 to 12; 

begin
	


Matrix_configuration_proc: process (clk, nReset, read_ID_ena)

begin
 
	if (not  nReset= '1') then
		IN_LEMO_cnt <=0;
		IN_OPT_I_cnt <=0;
        IN_OPT_O_cnt <=0;
        
        Matrix_conf <=(others => '0'); -- 
        for i in 1 to 12 loop
            conf_reg(i)<= (others => '0' );
        end loop; 
    elsif (clk'EVENT AND clk = '1') then
		if read_ID_ena ='1' then 

				conf_reg(1)<= slave1_ID;
            	conf_reg(2)<= slave2_ID;
           	 	conf_reg(3)<= slave3_ID;
            	conf_reg(4)<= slave4_ID;
            	conf_reg(5)<= slave5_ID;
                conf_reg(6)<= slave6_ID;
            	conf_reg(7)<= slave7_ID;
           	 	conf_reg(8)<= slave8_ID;
            	conf_reg(9)<= slave9_ID;
                conf_reg(10)<= slave10_ID;
                conf_reg(11)<= slave11_ID;
            	conf_reg(12)<= slave12_ID;

                for i in 1 to 12 loop
                    case conf_reg(i) is
                        when "01000001" => 
                                           IN_LEMO_prot_cnt <= IN_LEMO_prot_cnt +1;

                        when "01000010" => 
                                           IN_OPT_prot_cnt <= IN_OPT_prot_cnt +1;

                        when "01001010" => 
                                           IN_LEMO_cnt <= IN_LEMO_cnt +1;

                        when "01001011" => 
                                           IN_OPT_I_cnt <= IN_OPT_I_cnt +1;

                        when "01001100" => 	
                                           IN_OPT_O_cnt <= IN_OPT_O_cnt +1;

                        when others     =>  NULL;
                    end case;
                end loop;

			
                if IN_LEMO_prot_cnt = 3 and IN_OPT_prot_cnt =2 then 
			
					Trigger_matrix_Config <= "00000001";
				else
                    if IN_LEMO_cnt=9 and IN_opt_O_cnt = 3 then
                        Trigger_matrix_Config <= "00000010"; --standard Matrix
                    else 
                        if IN_LEMO_cnt=0 and IN_opt_I_cnt = 9 and IN_opt_O_cnt =1 then 
                            Trigger_matrix_Config  <= "00000011";--optical Matrix
                        else
                            if IN_LEMO_cnt=7 and IN_opt_I_cnt = 2 and IN_opt_O_cnt =3 then
                                Trigger_matrix_Config  <= "00000100";--mixed input Matrix
                            else
                                Trigger_matrix_Config <= "00000000"; --
                            end if;
                        end if;
                    end if;
                end if;
		end if;
	end if;
    end process Matrix_configuration_proc; 

end architecture qud_trig_matrix_arch;