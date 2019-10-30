package file_access is

  procedure file_access_init(pts : integer);
  attribute foreign of file_access_init : procedure is "VHPIDIRECT file_access_init";

  function file_access_read(timeout : integer) return integer;
  attribute foreign of file_access_read : function is "VHPIDIRECT file_access_read";

  procedure file_access_write(x : integer);
  attribute foreign of file_access_write : procedure is "VHPIDIRECT file_access_write";

  procedure file_access_flush;
  attribute foreign of file_access_flush : procedure is "VHPIDIRECT file_access_flush";


  shared variable my_var : integer := 43;
end package;

package body file_access is
  procedure file_access_init(pts : integer) is
  begin
    assert false report "VHPI" severity failure;
  end procedure;

  function file_access_read(timeout : integer) return integer is
  begin
    assert false report "VHPI" severity failure;
    return 0;
  end function;

  procedure file_access_write(x : integer) is
  begin
    assert false report "VHPI" severity failure;
  end procedure;

  procedure file_access_flush is
  begin
    assert false report "VHPI" severity failure;
  end procedure;

end package body;