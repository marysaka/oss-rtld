.section ".rodata.mod0"
.global __nx_mod0
__nx_mod0:
    .ascii "MOD0"
    .word  __dynamic_start__      - __nx_mod0
    .word  __bss_start__          - __nx_mod0
    .word  __bss_end__            - __nx_mod0
    .word  __eh_frame_hdr_start__ - __nx_mod0
    .word  __eh_frame_hdr_end__   - __nx_mod0
    .word SELF_MODULE_RUNTIME     - __nx_mod0
