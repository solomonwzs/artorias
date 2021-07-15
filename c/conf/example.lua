as_config = {
  ["tcp_port"] = 5555,
  ["n_workers"] = 8,
  ["conn_timeout"] = 5,

  ["worker_type"] = 2,
  ["lua_cpath"] = "./bin/?.so",
  ["lua_path"] = "./lua/?.lua",
  ["worker"] = "./test/t_simple_proxy.lua"
}
