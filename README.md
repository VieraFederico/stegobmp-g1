# stegobmp-g1
Academic and Investigative proyect to learn about Stenography and various encryption methods.


## Probar
### embed
./stegobmp -embed -in secreto.txt -p imagen24.bmp -out imagen_out.bmp -steg LSB1

### extract
./stegobmp -extract -p imagen_out.bmp -out recuperado.txt -steg LSB1