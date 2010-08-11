'.base': {
    '#kern': {
        'vnode_patch':  '@ - 08 00 10 00', # must be 1st result
        'ovbcopy':      '+_ovbcopy',
        'scratch':      '!scratch',

        'patch4':       '-_PE_i_can_has_debugger',
        'patch4_to':    0x47702001,

        # Do this just long enough to setuid(0) and sysctl
        'patch_suser': '-_suser',
        'patch_suser_to': 0x47702000,
        'patch_suser_orig': '*<patch_suser>',

        'patch_proc_enforce': '!sysctl:proc_enforce',
        'patch_proc_enforce_to': 0,

        'patch_cs_enforcement_disable': '~ @ 00 00 00 00 00 00 00 - 00 00 00 00 01 00 00 00 80',
        'patch_cs_enforcement_disable_to': 1,

        #'patch_sandbox': '- f0 b5 03 af 2d e9 00 0d c1 b0 82 46',
        #'patch_sandbox_to': 0x47702000,
        #'patch_sandbox_orig': 0xaf03b5f0,

        'mac_policy_list': '*(-_mac_label_get-4)',

        # for pmap.c
        'kernel_pmap':  '-_kernel_pmap',
        'mem_size':     '-_mem_size',

        'strncmp': '+_strncmp',
        'vn_getpath': '+_vn_getpath',
        'mpo_base': '!mpo_base',
        'mpo_vnode_check_access_ptr': '<mpo_base>+(252<<2)',
        'mpo_vnode_check_open_ptr':   '<mpo_base>+(267<<2)',
        'mpo_vnode_check_access':     '*<mpo_vnode_check_access_ptr>',
        'mpo_vnode_check_open':       '*<mpo_vnode_check_open_ptr>',

        'rgbout': '!rgbout',
        
        'adjusted_vram_baseaddr': ('vram_baseaddr', 'e1'),

        'current_thread': '+_current_thread',
        'ipc_kobject_server_end': '!stringref:"ipc_kobject_server: strange destination rights',
        'ipc_kobject_server_start': '!bof:<ipc_kobject_server_end>',
        
        'root_ios_id': 1,
    },
    '#cache': {
        # for installui.m
        'ft_path_builder_create_path_for_glyph': '-_ft_path_builder_create_path_for_glyph',
        'get_glyph_bboxes': '-_get_glyph_bboxes',
    },

    'kill_sb': 0,
},

'.armv6': {
    '<':            '.base',
    'arch':         'armv6',

    'is_armv7': 0,
    
    '#launchd': {
        # mov r0, #1; bx lr
       -1:              '@ - 01 00 a0 e3 1e ff 2f e1',
        # ldr r0, [r0] -> _launch_data_new_errno
        0:              '- 00 00 90 e5 .. .. .. .. 04 20 a0 e1',
        # lsr r0, r0, #2 -> _setrlimit
        1:              '05 10 a0 e1 - 20 01 a0 e1 .. .. .. .. 01 00 70 e3 .. .. .. .. .. .. .. .. 04 10 a0 e3',
        # add r0, #3 -> __exit
        2:              '01 00 00 .. - 03 00 80 e2',
        # ldmia r0, {r0-r3} -> _audit_token_to_au32
        3:              '14 00 8c e2 - 0f 00 90 e8',
        # str r2, [sp, #4] -> _launch_data_unpack
        4:              '02 30 a0 e1 - 04 20 8d e5',
        # str r3, [sp, #8] -> _launch_data_dict_iterate
        5:              '0d 20 a0 e1 - 08 30 8d e5',
        # pop {r4, r7, pc}
        6:              '@ - 90 80 bd e8',
        # sub.w sp, r7, #0xc; pop {r4-r7, pc}
        7:              '@ - 0c d0 47 e2 f0 80 bd e8',

        # mov r0, r4; mov r1, r6; mov r2, r5;
        # blx _strlcpy; pop {r4-r7, pc}
       -8:              '- 04 00 a0 e1 06 10 a0 e1 05 20 a0 e1 .. .. .. .. f0 80 bd e8',
        # str r0, [r5]; pop {r4-r7, pc}
       -9:              '@ - 00 00 85 e5 b0 80 bd e8',
        # ldr r0, [r4]; blx _pthread_detach
       10:              '00 00 50 e3 .. .. .. .. - 00 00 94 e5 .. .. .. .. 00 10 50 e2',
        # mov r0, r6; pop {r4-r7, pc}
       11:              '@ - 06 00 a0 e1 f0 80 bd e8',
    },

    '#cache': {
        # ldr r0, [r0]; pop {r4, r5, r7, pc}
        'k4': '@ - 00 00 90 e5 b0 80 bd e8',
        # str r0, [r4]; pop {r4, r7, pc}
        'k5': '@ - 00 00 84 e5 90 80 bd e8',
        # add r0, r4; pop {r4, r7, pc}
        'k6': '@ - 00 00 84 e0 90 80 bd e8',
        # pop {r0-r3, pc} (ARM!)
        'k7': '@ - 0f 80 bd e8',
        # pop {r1, r2, pc}
        'k8': '@ + 06 bd',
        # pop {r1, r2, r3, pc}
        'k9': '@ + 0e bd',
        # sub sp, r7, #0; pop {r7, pc}
        'k10': '@ - 00 d0 47 e2 80 80 bd e8',
        # pop {r4-r7, pc}
        'k11': '@ + f0 bd',
        # pop {r4, pc}
        'k18': '@ + 10 bd',
        # blx r4; pop {r4, r5, r7, pc}
        'k12': '@ - 34 ff 2f e1 b0 80 bd e8',
        
        # add r0, sp, #600; pop {r1, r7, pc}
        'k13': '@ + 96 a8 82 bd',
        # ldr r0, [r4, r0]; pop {r4, r7, pc}
        'k14': '@ + 20 58 90 bd',

        # str r4, [r0]; pop {r4, r7, pc}
        'k15': '@ - 00 40 80 e5 90 80 bd e8',

        # ldr r0, [r4]; pop {r4, r7, pc}
        'k16': '@ - 00 00 94 e5 90 80 bd e8',
    },

    '#kern': {
        'patch1':       '- .. .. .. .. 6b 08 1e 1c eb 0a 01 22 1c 1c 16 40 14 40',
        'patch1_to':    0x46c046c0,
        'patch3':       '13 20 a0 e3 .. .. .. .. 33 ff 2f e1 00 00 50 e3 00 00 00 0a .. 40 a0 e3 - 04 00 a0 e1 90 80 bd e8',
        'patch3_to':    0xe3a00001,

        'patchkmem0': '- 1b 68 00 2b .. .. .. .. .. .. a5 68 e6 68 00 23',
        'patchkmem0_to': 0x46c02301,

        'patchkmem1': '1b 68 - 00 2b .. .. a3 68 .. .. .. .. 5b 18 93 42 cc',
        'patchkmem1_to': 0x46c046c0,

        'patch_nosuid': '6a/62 5a .. .. - 13 40 63/6b 52 95 23',
        'patch_nosuid_to': 0x46c046c0,
        
        'e0': '@ + 00 bd', # pop {pc}
        # sub sp, r7, #0; pop {r7, pc}
        'e1': '@ - 00 d0 47 e2 80 80 bd e8',
        'e2': '+ 01 a8 0f/1b 99 00 9a',

    },

},

'.armv6_4.x': {
    '<': '.armv6',
    '#cache': {
        'magic_offset': -960,
    }
},

'.armv6_3.1.x': {
    '<': '.armv6',
    '#cache': {
        'magic_offset': -956,
    }
},

'.armv7': {
    '<':            '.base',
    'arch':         'armv7',
    
    'is_armv7': 1,

    '#launchd': {
        # mov r0, #1; bx lr
       -1:              '@ + 01 20 70 47',
        # ldr r0, [r0] -> _launch_data_new_errno
        0:              '+ 00 68 .. .. .. .. 22 46 01 34',
        # lsr r0, r0, #2 -> _setrlimit
        1:              '60 69 29 46 + 80 08',
        # add r0, #3 -> __exit
        2:              'b0 f1 ff 3f .. .. + 03 30 16 f0',
        # ldmia r0, {r0-r3} -> _audit_token_to_au32
        3:              '8d e8 0f 00 0c f1 14 00 + 0f c8',
        # str r2, [sp, #4] -> _launch_data_unpack
        4:              '02 98 00 93 59 46 13 46 + 01 92',
        # str r3, [sp, #8] -> _launch_data_dict_iterate
        5:              '6a 46 01 93 01 33 + 02 93',
        # pop {r4, r7, pc}
        6:              '@ + 90 bd',
        # sub.w sp, r7, #0xc; pop {r4-r7, pc}
        7:              '@ + a7 f1 0c 0d f0 bd',
    },
    
    '#cache': {
        # ldr r0, [r0]; pop {r4, r5, r7, pc}
        'k4': '@ + 00 68 b0 bd',
        # str r0, [r4]; pop {r4, r7, pc}
        'k5': '@ + 20 60 90 bd',
        # add r0, r4; pop {r4, r7, pc}
        'k6': '@ + 20 44 90 bd',
        # pop {r0-r3, pc}
        'k7': '@ + 0f bd',
        # pop {r0-r3, pc} (ARM!)
        #'k7': '@ - 0f 80 bd e8',
        # pop {r1, r2, pc}
        'k8': '@ + 06 bd',
        # pop {r1, r2, r3, pc}
        'k9': '@ + 0e bd',
        # sub sp, r7, #0; pop {r7, pc}
        'k10': '@ + a7 f1 00 0d 80 bd',
        # pop {r4-r7, pc}
        'k11': '@ + f0 bd',
        # pop {r4, pc}
        'k18': '@ + 10 bd',
        # blx r4; pop {r4, r5, r7, pc}
        'k12': '@ + a0 47 b0 bd',

        # add r0, sp, #600; pop {r1, r7, pc}
        'k13': '@ + 96 a8 82 bd',
        # ldr r0, [r4, r0]; pop {r4, r7, pc}
        'k14': '@ + 20 58 90 bd',

        # str r4, [r0]; pop {r4, r7, pc}
        'k15': '@ + 40 f8 04 4b 90 bd',

        # ldr r0, [r4]; pop {r4, r7, pc}
        'k16': '@ + 20 68 90 bd',
    },

    '#kern': {
        'storedude': '+ 43 6a 00 20 13 60 70 47', # this is only for USB crap

        'patch1':       '- 02 0f .. .. 63 08 03 f0 01 05 e3 0a 13 f0 01 03 1e 93',
        'patch1_to':    0x46c00f02,
        'patch3':       '23 78 9c 45 05 d1 .. .. .. .. .. .. .. 4b 98 47 00 .. -',
        'patch3_to':    0x1c201c20,
        
        # It would be better to patch setup_kmem itself but this is easier.
        'patchkmem0':   '1b 68 00 2b - .. .. .. .. .. .. 04 f1 08 05',
        'patchkmem0_to': 0x46c046c0,
        # Note: the 1a 68 in tense3 is only because it was already patched
        # It is actually 1b 68, but I'm going to be lazy here
        'patchkmem1':   '.. 68 - 00 2b .. .. a3 68 2a/2b/2f 4a/49',
        'patchkmem1_to': 0x46c046c0,

        # search for bic.*0xc00, or vnode_authorize
        'patch_nosuid': 'd6/d8 f8 88 30 db 6b - 13 f0 08 0f',
        'patch_nosuid_to': 0x0f00f013, # tst r3, #0

        'e0': '@ + 00 bd', # pop {pc}
        'e1': '@ + a7 f1 00 0d 80 bd', # sub sp, r7, #0; pop {r7, pc}
        'e2': '+ 01 a8 0f/1b 99 00 9a',
    },
},

'.armv7_4.x': {
    '<': '.armv7',
    '#cache': {
        #...
        'magic_offset': -964,
    },
},

'.armv7_3.1.x': {
    '<': '.armv7',
    '#kern': {
        'patch1':       '- 02 0f 40 f0 .. .. 63 08 03 f0 01 05 e3 0a 13 f0 01 03 1e 93',
        'patch1_to':    0xf0400f00,
        'patch3':       '61 1c 70 46 13 22 05 4b 98 47 - 00 ..',
        'patch3_to':    0x46c046c0,
    },
    '#cache': {
       'magic_offset': -960, 
    },
},

'iPad1,1_3.2': {
    '<': '.armv7',
    '#kern': {
        'vram_baseaddr': [0xed6ed000, 0xed6f5000],
    },
    '#cache': {
        'magic_offset': -960,
    },
    'kill_sb': 1,
},
'iPad1,1_3.2.1': {
    '<': 'iPad1,1_3.2',
},

'iPad1,1_3.2_self': {
    '<': 'iPad1,1_3.2',
    '#cache': { '@binary': '/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv7', },
    '#kern': { '@binary': '/var/mobile/ipadkern', },
    '#launchd': { '@binary': '/sbin/launchd', },
},

'iPhone3,1_4.0': {
    '<': '.armv7_4.x',
    '#kern': {
        'vram_baseaddr': 0xd2675000,
        # ???
        'patch3':       '70 46 13 22 .. 4b 98 47 00 .. -',
        'root_ios_id': 2,
    },
},
'iPhone3,1_4.0.1': { '<': 'iPhone3,1_4.0', },
'iPhone1,x_4.0': {
    '<': '.armv6_4.x',
    '#kern': {
        'vram_baseaddr': 0xca67d000,     
    },
},
'iPhone1,x_4.0.1': {
    '<': '.armv6_4.x',
    '#kern': {
        'vram_baseaddr': 0xca67d000, 
    },
},
'iPhone2,1_4.0': {
    '<': '.armv7_4.x',
    '#kern': {
        'vram_baseaddr': 0xcd32d000, 
    },
},
'iPhone2,1_4.0.1': {
    '<': '.armv7_4.x',
    '#kern': {
        'vram_baseaddr': 0xcd32d000,    
    },
},
'iPod2,1_4.0': {
    '<': '.armv6_4.x',
    '#kern': {
        'vram_baseaddr': 0xca68d000, 
    },
},
'iPhone2,1_3.1.3': {
    '<': '.armv7_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xed731000,       
    },
},
'iPhone2,1_3.1.2': {
    '<': '.armv7_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xed731000,       
    },
},
'iPod2,1_3.1.2': {
    '<': '.armv6_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xeb849000,
    },
},
'iPod2,1_3.1.3': {
    '<': '.armv6_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xed731000,
    },
},
'iPhone1,x_3.1.3': {
    '<': '.armv6_3.1.x',
    '#kern': {
        'vram_baseaddr': [0xeb811000, 0xeb809000],
    },
},
'iPhone1,x_3.1.2': {
    '<': 'iPhone1,x_3.1.3',
    '#kern': {
        'vram_baseaddr': [0xeb811000, 0xeb809000],
    },
},
'iPod3,1_3.1.3': {
    '<': '.armv7_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xed615000, 
    },
},
'iPod3,1_3.1.2': {
    '<': '.armv7_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xed615000,       
    },
},
'iPod1,1_3.1.2': {
    '<': '.armv6_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xeb919000,   
    },
},
'iPod1,1_3.1.3': {
    '<': '.armv6_3.1.x',
    '#kern': {
        'vram_baseaddr': 0xeb919000,   
    },
},
'iPod3,1_4.0': {
    '<': '.armv7_4.x',
    '#kern': {
        'vram_baseaddr': 0xcd33d000, 
    },
},
