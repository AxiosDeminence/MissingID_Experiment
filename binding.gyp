{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "src/missing_id_napi.c" ],
      "include_dirs": [
        "include"
      ],
      "libraries": [
        "<(module_root_dir)/lib/missing_id.so"
      ]
    }
  ]
}