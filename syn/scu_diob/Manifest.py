target = "altera"
action = "synthesis"

fetchto = "../../ip_cores"

syn_device = "ep2agx125ef"
syn_grade = "c5"
syn_package = "29"
syn_top = "scu_diob"
syn_project = "scu_diob"

modules = {
  "local" : [ 
    "../../top/scu_diob/", 
  ]
}
