		TITLE	EOIND - Copyright (c) SLR Systems 1994

		INCLUDE MACROS
		INCLUDE	IO_STRUC

		PUBLIC	END_OF_INDIRECT


		.DATA

		EXTERNDEF	IND_DEVICE:DWORD


		.CODE	PHASE1_TEXT

		EXTERNDEF	RELEASE_IO_SEGMENT:PROC,_close_handle:proc


END_OF_INDIRECT	PROC
		;
		;
		;
		PUSH	EBX
		XOR	ECX,ECX
		MOV	EBX,IND_DEVICE
		ASSUME	EBX:PTR MYI_STRUCT

		RESS	INDIRECT_MODE,CL
		TEST	EBX,EBX
		JZ	EOI_1
		MOV	EAX,[EBX].MYI_BLOCK
		MOV	[EBX].MYI_BLOCK,ECX
		TEST	EAX,EAX
		JZ	L1$
		CALL	RELEASE_IO_SEGMENT
L1$:
		MOV	EAX,[EBX].MYI_HANDLE
		MOV	[EBX].MYI_HANDLE,ECX
		MOV	IND_DEVICE,ECX
		TEST	EAX,EAX
		JZ	EOI_1

		push	EAX
		call	_close_handle
		add	ESP,4
EOI_1:
		POP	EBX
		RET

END_OF_INDIRECT	ENDP


		END

