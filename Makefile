# Script Makefile para compilação do driver Linux para controle de um display de 7 segmentos
# Autor: Lucas Barboza

# Declaramos que o nosso arquivo .c será compilado e transformado em um objeto (arquivo .o)
# Este objeto será então transformado em um arquivo especial chamado de Kernel Object (arquivo .ko),
# que poderá ser carregado para o nosso Kernel. Este é o produto final do nosso driver após a compilação
obj-m += sevenseg.o

# Abaixo temos as regras de compilação. O sistema Make é parecido com uma receita de bolo, colocamos
# os comandos a serem executados em cada receita e estes comandos serão executados quando chamados.
# Neste caso, o nosso Makefile irá rodar um make por baixo dos panos para compilar nosso sevenseg.o
# junto com os outros módulos do Kernel (make recursivo).
# OBS.: a receita 'all' será executada por padrão ao se chamar o comando 'make'
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# A receita 'clean' serve para remover tudo o que foi compilado e voltar nossa pasta ao estado original
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	
