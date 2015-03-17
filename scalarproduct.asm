%ifdef ARCH_X86_64
    %ifidn __OUTPUT_FORMAT__,win64
        %define WIN64
    %else
        %define UNIX64
    %endif
%else
; x86_32 doesn't require PIC.
; Some distros prefer shared objects to be PIC, but nothing breaks if
; the code contains a few textrels, so we'll skip that complexity.
    %undef PIC
%endif

%ifidn __OUTPUT_FORMAT__,elf
%define BINFMT_IS_ELF
%elifidn __OUTPUT_FORMAT__,elf32
%define BINFMT_IS_ELF
%elifidn __OUTPUT_FORMAT__,elf64
%define BINFMT_IS_ELF
%endif

%ifidn __OUTPUT_FORMAT__,win32
    %define mangle(x) _ %+ x
%else
    %define mangle(x) x
%endif

%ifdef WIN64 ; Windows x64 ;=================================================

%define r0 rcx
%define r1 rdx
%define r2 r8
%define r3 r9
%define r4 rdi
%define r5 rsi

%elifdef ARCH_X86_64 ; *nix x64 ;=============================================

%define r0 rdi
%define r1 rsi
%define r2 rdx
%define r3 rcx
%define r4 r8
%define r5 r9

%else ; X86_32 ;==============================================================

%define r0 ecx
%define r1 edx
%define r2 ebx
%define r3 esi
%define r4 edi
%define r5 eax
%define rbp ebp
%define rsp esp

%endif ;======================================================================

;=============================================================================
; arch-independent part
;=============================================================================

; This is needed for ELF, otherwise the GNU linker assumes the stack is
; executable by default.
%ifdef BINFMT_IS_ELF
SECTION .note.GNU-stack noalloc noexec nowrite progbits
%endif

section .text align=16

;-----------------------------------------------------------------------------
; float scalarproduct_fir_float(const float *v1, const float *v2, uint32_t len)
;-----------------------------------------------------------------------------
%xdefine scalarproduct_fir_float_sse mangle(scalarproduct_fir_float_sse)
    global scalarproduct_fir_float_sse
%ifdef BINFMT_IS_ELF
    [type scalarproduct_fir_float_sse function]
%endif
    align 16 
scalarproduct_fir_float_sse:
    %assign stack_offset 0
	push rbp
	mov rbp, rsp
    push r4
%ifndef ARCH_X86_64
    push r2
    push r3
    %assign stack_offset stack_offset+12
    mov r0, [esp + stack_offset +  8]
    mov r1, [esp + stack_offset + 12]
    mov r2, [esp + stack_offset + 16]
%endif
    shl        r2, 2
    lea        r4, [2*r2-0x08]
    xor        r3, r3
    xorps    xmm0, xmm0
.loop:
    movups   xmm1, [r0+r3]
    movups   xmm2, [r0+r4]
    shufps   xmm2, xmm2, 0x1b
    addps    xmm1, xmm2
    movaps   xmm3, [r1+r3]
    mulps    xmm1, xmm3
    addps    xmm0, xmm1
    add        r3, 0x10
    sub        r4, 0x10
    cmp        r3, r2 
    jl .loop
    movaps   xmm1, xmm0
    shufps   xmm1, xmm0, 0x1b
    addps    xmm1, xmm0
    movhlps  xmm0, xmm1
    addps    xmm0, xmm1
    movss    xmm2, [r0+r2]
    addss    xmm0, xmm2
%ifndef ARCH_X86_64
    movss     [esp + stack_offset + 8],  xmm0
    fld dword [esp + stack_offset + 8] 
    pop r3
    pop r2
%endif
    pop r4
    pop rbp
    ret
%ifdef BINFMT_IS_ELF
.endfunc
[size scalarproduct_fir_float_sse scalarproduct_fir_float_sse.endfunc-scalarproduct_fir_float_sse]
%endif

