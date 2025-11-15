# stegobmp — Trabajo Práctico de Esteganografía

Este proyecto implementa un programa de esteganografía para ocultar y extraer archivos dentro de imágenes BMP de 24 bits, cumpliendo con los requisitos del Trabajo Práctico de la materia Criptografía y Seguridad (72.04) del ITBA.

El programa soporta los métodos LSB1, LSB4 y LSBI, y cuenta con encriptación opcional mediante AES (128/192/256) y 3DES en modos CBC, CFB y OFB.

# Objetivos del proyecto

Introducir conceptos de esteganografía digital.

Implementar métodos de ocultamiento en imágenes BMP.

Comparar LSB1, LSB4 y LSBI.

Incorporar cifrado simétrico para mayor seguridad.

Extraer información oculta y realizar estegoanálisis.

# Uso del programa
*Ocultar un archivo (embed)*
./stegobmp -embed \
  -in secreto.txt \
  -p imagen.bmp \
  -out salida.bmp \
  -steg LSB1

*Ocultar un archivo con encriptación*
./stegobmp -embed \
  -in secreto.txt \
  -p imagen.bmp \
  -out salida.bmp \
  -steg LSBI \
  -a aes256 \
  -m cbc \
  -pass "claveSegura"

*Extraer un archivo (extract)*
./stegobmp -extract \
  -p salida.bmp \
  -out recuperado.txt \
  -steg LSB4

*Extraer con desencriptado*
./stegobmp -extract \ 
  -p salida.bmp \
  -out recuperado.txt \
  -steg LSBI \
  -a 3des \
  -m ofb \
  -pass "claveSegura"

## Algoritmos implementados
### LSB1
Inserta 1 bit del mensaje en el bit menos significativo de cada componente RGB.
### LSB4
Inserta 4 bits por componente de color. Mayor capacidad, mayor impacto visual.
### LSBI (Least Significant Bit Improved)

Implementación basada en el paper de Majeed & Sulaiman.
Este método busca reducir la cantidad de cambios perceptibles mediante una estrategia adaptativa.

Características principales:

- Detecta patrones de 2 bits en los componentes del BMP.
- Evalúa cuántos cambios provoca insertar los bits del mensaje.
- Decide, por patrón, si conviene invertir los bits o no para minimizar cambios.
- Inserta el mensaje usando esa decisión adaptativa.
- Registra el “pattern map” utilizado en los primeros bytes del archivo.
- Reduce la distorsión visual en comparación con LSB1 y LSB4.

## Encriptación

El programa permite cifrar el archivo antes de esteganografiarlo.

Algoritmos:
- AES128 / AES192 / AES256
- 3DES
Modos:
- CBC
- CFB
- OFB

Derivación de clave:
- PBKDF2 con salt fijo (requisito del TP)
- El archivo se cifra completo, y luego se oculta junto con:

## Testing y validación (incluye LSBI)

El proyecto incluye un script test.sh utilizado para validar:

LSB1 / LSB4 / LSBI

Embed y extract con y sin cifrado

Integridad tras encriptar–ocultar–extraer–desencriptar

Manejo correcto del pattern map LSBI

Capacidades y errores del BMP

Padding y modos de cifrado

Este script fue usado intensivamente para garantizar el correcto funcionamiento del algoritmo LSBI, que es el más sensible a errores.
#  Estructura del proyecto

```plaintext
├── bmp_handler/          Lectura y escritura del BMP
├── common/               Representación interna de la imagen
├── encryption_manager/   AES / 3DES + modos CBC/CFB/OFB
├── lsb1/                 Implementación del método LSB1
├── lsb4/                 Implementación del método LSB4
├── lsbi/                 Implementación del método LSBI
├── utils/
│   ├── parser/           Procesamiento de argumentos CLI
│   ├── operations/       Lógica de embed y extract
│   ├── file_management/  Manejo de archivos
│   └── translator/       Utilidades adicionales
└── main.c                Punto de entrada del programa
```
### Compilación

Requisitos:
* gcc
* make
* libssl-dev

Compilar:
* make

Ejecutar:
* ./stegobmp

Implementado como parte del Trabajo Práctico de Criptografía y Seguridad (72.04) — ITBA.
Uso académico y educativo.