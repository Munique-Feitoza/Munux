# Munux Core

Este é o núcleo do sistema operacional Munux, com um bootloader simples escrito em Assembly.

## Sobre o Bootloader

- O bootloader é carregado pela BIOS na memória no endereço `0x7C00`.
- Ele imprime uma mensagem na tela `"Iniciando Munux..."`.
- Após imprimir, trava em um loop infinito para fins de teste.
- O código está escrito em Assembly 16 bits (modo real).
- Contém a assinatura `0xAA55` para ser reconhecido como bootloader válido.

## Estrutura do Projeto

- `bootloader.asm`: código-fonte do bootloader.
- `Makefile`: automatiza a compilação e execução no QEMU.
- `bootloader.bin`: arquivo binário gerado pelo NASM.
- `kernel.c` (em desenvolvimento): código do kernel em C.
- `linker.ld` (em desenvolvimento): script de linkagem para o kernel.
- `iso/boot/grub/grub.cfg` (em desenvolvimento): configuração do GRUB.

## Como compilar e testar

1. Compile o bootloader com:
```
make
```

2. Execute no QEMU com:
```
make run
```

## Próximos passos

    * Desenvolver o kernel em C.

    * Integrar bootloader com o kernel.

    * Criar uma imagem ISO bootável com GRUB.

---

## Licença

Este projeto está licenciado sob a GPLv3.
