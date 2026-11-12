[bits 16]

%if 0
 FILENAME:      ./bullshit.s
 NAME:          SBL, Stage 2 Functions, specifically the ones regarding the disk
 DESCRIPTION:   Disk driver wrappers for SBL. Only here to style on these hoes;
                otherwise I wouldn't bother copy-pasting it out of the s2func.s
                file it originated from.
 AUTHOR:        Renee Innogen Graves
 DATE:          12 November 2026
 COPYRIGHT:     Renee Innogen Graves (notably a lot of this is still MIT license)
 LICENSE:       MIT License (./LICENSE)
 NOTE:          so much make bullshit please im gonna fucking break 
                every one of my bones
%endif

; renamed arg_segment because segment is an actual nasm keyword
%define arg_segment [bp + 0x08]
; renamed arg_offset to match arg_segment
%define arg_offset [bp + 0x06]

global GetSector, GetDriveParameters

GetDriveParameters:
    ; variables:
    ; [bp + 0x00]: original base pointer (NOT A VARIABLE DO NOT FUCKING USE)
    ; [bp + 0x02]: return address (see warning above)
    ; [bp + 0x06]: offset (lower half of uint32_t segment:offset pair)
    ; [bp + 0x08]: segment (upper half of uint32_t segment:offset pair)
    ; [bp + 0x0A]: drive number (uint8_t)

    push bp
    mov bp, sp
    push es
    push di
    push bx
    push ds
    push si

    mov ds, arg_segment
    mov si, arg_offset
    mov dl, [bp + 0x0A] ; drive number
    mov ah, 0x48
    int 0x13

    mov al, ah
    xor ah, ah

    pop si
    pop ds
    pop bx
    pop di
    pop es
    pop bp
    o32 ret

GetSector:
    ; variables:
    ; [bp + 0x00]: original base pointer (NOT A VARIABLE DO NOT FUCKING USE)
    ; [bp + 0x02]: return address (see warning above)
    ; [bp + 0x06]: offset (lower half of uint32_t segment:offset pair)
    ; [bp + 0x08]: segment (upper half of uint32_t segment:offset pair)
    ; [bp + 0x0A]: drive number (uint8_t)

    push bp
    mov bp, sp
    push es
    push di
    push bx
    push ds
    push si

    mov ds, arg_segment
    mov si, arg_offset
    mov dl, [bp + 0x0A] ; drive number
    mov ah, 0x42
    int 0x13

    mov al, ah
    xor ah, ah
    
    .GetSector_END:
        pop si
        pop ds
        pop bx
        pop di
        pop es
        pop bp
        o32 ret ; could've just done rets everywhere else this is stupid