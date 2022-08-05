# FechaduraEletronicaREDE
# Fechadura Eletrônica com Abertura por RFID ou LOGIN / SENHA com registro centralizado em bd MySql.

Prosposta desse sistema é automatizar entradas/e ou saída de edificações com acionamento via cartão/tag RFID, 
seguido ou não de entrada de senha via teclado númérico. 

1) Usuário aproxima tag RFID de sensor;
2) Módulo de leitura conectado a rede ethernet faz pesquisa em bd MYSQL;
3) Apresenta mensagem customizada bem-vindo [Fulano] ao portador da tag;
4) Portador da tag regista senha de acesso [opcional]
5) Sistema permite o acesso desbloqueando porta eletromagnética;
6) Log é resgistrado em banco, pessoa, data e hora e porta de acesso.

tecnologias: arduino + módulo ethernet, linguagem C, atuadores (ex.: eletroímã).


@Author: Deividson Calixto da Silva.
