# Exercício 2

A segunda parte do projeto é relativa à construção de um sistema de publicação e subscrição de mensagens, tendo por base o
sistema de ficheiros criado na primeira parte do projeto (ou alternativamente, usando o novo [código base](https://github.com/tecnico-so/projeto-so-2022-23/tree/main)).

Existem quatro tipos de entidades envolvidas neste projeto:
- Um servidor _mbroker_, cujo objetivo é a gestão de processos cliente para enviar e receber mensagens, bem como criar e destruir canais;
- Um gestor de canais _manager_, que envia comandos ao servidor relativos à criação e destruição de canais de comunicação entre clientes;
- Um cliente do tipo _publisher_, que publica mensagens para um determinado canal de comunicação;
- Um (ou vários) clientes do tipo _subscriber_, que subscrevem-se a um determinado canal de comunicação, ficando à espera de receber mensagens do _publisher_;

O enunciado desta fase do projeto encontra-se neste [link](https://github.com/tecnico-so/enunciado-projeto-so-2022-23/blob/main/exercise2.md).

## Como correr o projeto

Num primeiro terminal que tenha o diretório deste exercício como base, compile o projeto usando o Makefile:

```
make
```

Para cada um dos tipos de entidades, navegue até à respetiva pasta antes de executar o respetivo comando.

### _mbroker_

O _mbroker_ é corrido com o seguinte comando:

``` 
./mbroker <register_pipe_name> <max_sessions>
```
em que *register_pipe_name* é o nome do canal usado para os clientes iniciarem o contacto com o servidor, e *max_sessions* é o número máximo de sessões que podem existir em concorrência no servidor.

### _manager_

O _manager_ é executado de uma das seguintes formas, dependendo da operação que se pretende executar relativamente aos canais de mensagens do servidor (criação, destruição e listagem):

```
./manager <register_pipe_name> <pipe_name> create <box_name>
./manager <register_pipe_name> <pipe_name> remove <box_name>
./manager <register_pipe_name> <pipe_name> list
```
em que *register_pipe_name* é o nome do canal usado para contactar o servidor numa primeira fase, *pipe_name* é o nome do canal pelo qual são feitos os pedidos ao servidor, e *box_name* é o nome do canal sobre o qual se pretende executar a operação.

### _publisher_

O _publisher_ é executado com o seguinte comando:

```
./pub <register_pipe_name> <pipe_name> <box_name>
```
em que *register_pipe_name* é o nome do canal usado para contactar o servidor numa primeira fase, *pipe_name* é o nome do canal pelo qual são feitos os pedidos ao servidor, e *box_name* é o nome do canal sobre o qual se pretende fazer a comunicação entre clientes.

### _subscriber_

O _subscriber_ executa-se com o comando ilustrado abaixo:

```
sub <register_pipe_name> <pipe_name> <box_name>
```
em que *register_pipe_name* é o nome do canal usado para contactar o servidor numa primeira fase, *pipe_name* é o nome do canal pelo qual são feitos os pedidos ao servidor, e *box_name* é o nome do canal sobre o qual se pretende fazer a comunicação entre clientes.