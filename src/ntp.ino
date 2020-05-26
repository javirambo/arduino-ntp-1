/*
 * Comunicación con servidor NTP para obtener fecha y hora de Internet.
 * Setea el reloj interno del Arduino para ponerlo en hora.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <time.h>

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
unsigned int localPort = 80;        //  Puerto local para escuchar UDP
const int NTP_PACKET_SIZE = 48;     // La hora son los primeros 48 bytes del mensaje
byte packetBuffer[NTP_PACKET_SIZE]; // buffer para los paquetes
EthernetUDP Udp;                    // Una instancia de UDP para enviar y recibir mensajes

void setup()
{
    // configuro mi Ethernet Shield con W5100:
    Ethernet.init(10);

    Serial.begin(9600);
    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Fallo al configurar por DHCP");
        while (true)
            ; // Sin IP no podemos seguir...
    }
    Udp.begin(localPort);

    Serial.print("Mi IP es ");
    Serial.println(Ethernet.localIP());
}

// Envío una petición NTP al servidor por medio del nombre (usa DNS)
void sendNTPpacket(char *address)
{
    // todo el buffer en 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Inicializo el paquete que uso como peticion NTP:
    // para mas detalles ver: http://www.networksorcery.com/enp/protocol/ntp.htm

    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

void loop()
{
    Serial.println("Envío la petición al servidor NTP");
    sendNTPpacket("time.nist.gov");

    Serial.println("Esperamos 2 segundos la respuesta...");
    delay(2000);

    if (Udp.parsePacket())
    {
        Serial.println("Paquete recibido.");

        // Leer el paquete
        if (Udp.read(packetBuffer, NTP_PACKET_SIZE) > 0)
        {
            // la firma de tiempo empieza en el byte 40 y son dos palabras de 2 bytes cada una.
            // guardo las dos palabras de segundos y fracciones:
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

            // hora combino las dos partes en un único long entero sin signo:
            unsigned long segundosDesde1900 = highWord << 16 | lowWord;
            Serial.print("Segundos desde 1 Enero de 1900 = ");
            Serial.println(segundosDesde1900);

            // En Arduino se usa el sistema Unix (viejo método de Lenguaje C),
            // esto es segundos desde el 1 de enero de 1970.
            // Se usa set_system_time con el offset NTP_OFFSET para corregir la hora.

            // corro la zona horaria a Argentina (-3)
            set_zone(-3 * ONE_HOUR);
            set_system_time(segundosDesde1900 - NTP_OFFSET);

            // tomo la hora, la convierto a formato legible y la muestro:
            time_t timer = time(NULL);
            Serial.println(ctime(&timer));

            delay(3000);
        }
    }
    else
    {
        Serial.println("No se recibe respuesta!");
    }
}
