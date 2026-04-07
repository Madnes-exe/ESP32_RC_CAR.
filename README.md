# Samochodzik-RC-z-Kamerk-
OPIS:
Projekt autonomicznego samochodzika RC sterowanego przez Wi-Fi, opartego o mikrokontroler ESP32-S3 z wbudowaną obsługą kamery.
System umożliwia zdalne sterowanie pojazdem poprzez przeglądarkę internetową oraz podgląd obrazu w czasie rzeczywistym
Samochód wykorzystuje silnik prądu stałego (DC) do napędu tylnej osi, sterowany za pomocą mostka H, oraz serwomechanizm 
do kontroli skrętu przednich kół. ESP32 działa jako punkt dostępowy (Access Point), do którego użytkownik łączy się 
za pomocą smartfona. Interfejs webowy umożliwia sterowanie pojazdem za pomocą dwóch suwaków (jazda przód/tył oraz skręt lewo/prawo).

Kluczowe funkcjonalności:
Sterowanie pojazdem przez Wi-Fi (ESP32 jako Access Point)
Strona WWW hostowana na ESP32
Streaming obrazu z kamery w czasie rzeczywistym
Sterowanie:
prędkością (PWM, mostek H)
kierunkiem jazdy
kątem skrętu (serwo)
Zasilanie z baterii 7.4V + przetwornica step-down do 5V

Technologie i komponenty:
ESP32-S3 
Kamera OV2640
Mostek L298N 
Serwomechanizm MG-90S
Przetwornica DC-DC (step-down)
HTML / CSS / JavaScript (interfejs webowy)
Wi-Fi (tryb Access Point + HTTP server)

Architektura systemu:

Hardware:
ESP32-S3
Kamera
Silnik DC + mostek H
Serwo
Zasilanie (7.4V → 5V)

Firmware (ESP32):
Serwer HTTP
Obsługa kamery (stream)
Sterowanie PWM (silnik) 
Obsługa żądań z przeglądarki

Frontend:
Strona WWW (hostowana na ESP32)
Podgląd wideo
Suwaki sterujące


