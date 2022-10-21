{
    "targets": [
        {
            "target_name": "ethlib",
            "sources": [ "libeth.cc" ],
            "include_dirs" : [
 	 			"<!(node -e \"require('nan')\")"
			]
        }
    ],
}