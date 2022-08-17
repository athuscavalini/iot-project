# iot-project

## Rastreador Veicular utilizando uma rede LoRa-Mesh

### Requisitos:
- ESP8266;
- Módulo LoRa Ra-01 Ai Thiker;
- Conta na plataforma Thinger.io;

O esquemático para conectar o ESP ao módulo LoRa está na imagem `esquema.png`.

### Para utilizar:

Crie no Thinger.io:
- os dispositivos, cada um com um identificador único em hexadecimal com 5 caracteres;
- um databucket para cadas dispositivo com o mesmo identificador dele.

Altere o arquivo `project.ino`:
- incluindo nome e senha de uma rede WiFi com conexão à internet;
- incluindo um identificador único para o dispotivo, no formato hexadecimal, na variável id;
- incluindo o nome de usuário da plataforma Thinger.io;
- incluindo seu Bearer Token com acesso aos databuckets do Thinger.io;
- e definindo a taxa de atualização e a distância mínima percorrida para sincronização.

Após enviar o código para o dispotivo, certifique-se de que ele se conectará ao menos uma vez à internet para sincronizar o relógio com a rede NTP. Ele piscará o LED 3 vezes quando se conectar, mantendo apagado posteriormente e piscando uma vez sempre que receber um pacote via LoRa.
