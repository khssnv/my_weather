{ stdenv
, mkRosPackage
, robonomics_comm 
}:

mkRosPackage rec {
  name = "${pname}-${version}";
  pname = "my_weather_acl";
  version = "master";

  src = ./.;

  propagatedBuildInputs = [ robonomics_comm ];

  meta = with stdenv.lib; {
    description = "";
    homepage = http://github.com/khssnv/my_weather;
    license = licenses.bsd3;
    maintainers = [  ];
  };
}
