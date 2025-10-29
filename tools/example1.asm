; This example demonstrates forward reference resolution,
; where symbols like STACK_TOP and COUNT are used before they are defined.
; The assembler must perform multiple passes to resolve these.
        ORG 0x8000

STACK_SIZE      EQU 256
STACK_BASE      EQU STACK_TOP - STACK_SIZE

START:
                DI                      ; F3
                LD SP, STACK_TOP        ; 31 00 91
                LD A, 10101010b         ; 3E AA
                LD A, 2*8+1             ; 3E 11
                DS COUNT                ; DS 100 -> 100 bytes of 00

; --- Stack definition ---
                DS 10                   ; 10 bytes of 00
                ORG STACK_BASE
                DS STACK_SIZE, 0xFF     ; DS 256, 0xFF -> this will overwrite START code
STACK_TOP:                      
COUNT           EQU 100