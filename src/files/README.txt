lado.bmp se usa como portador en todos los casos
Todos tienen oculta una imagen .png
ladoLSB1.bmp
	con método LSB1 (sin encripción)
ladoLSB4.bmp
	con método LSB4 (sin encripción)
ladoLSBI.bmp
	con LSBI (sin encripción)
ladoLSB1aes128cbc.bmp
	con método LSB1, encriptado previamente con aes128, modo cbc,
	password "margarita" (derivando key e iv con el algoritmo PBKDF2, usando sha256, con salt 0x0000000000000000, 10000 iteraciones)
	
	Key derivada (32 bytes):03db0a157acfe8de523760aa731d8122
	IV derivada (16 bytes): 212420edc583a686a94d19a3497363a2

ladoLSBIaes256ofb.bmp
	con método LSBI, encriptado previamente con aes256, modo ofb, 
	password "margarita" (derivando key e iv con el algoritmo PBKDF2, usando sha256, con salt 0x0000000000000000, 10000 iteraciones)

	Key derivada (32 bytes):03db0a157acfe8de523760aa731d8122b25f8d99f3173ec0b52849f459a4c20d
	IV derivada (16 bytes): 212420edc583a686a94d19a3497363a2

ladoLSBIdescfb.bmp
	con método LSBI, encriptado previamente con des ede3 (triple des de tres claves), modo cfb8, 
	password "margarita" (derivando key e iv con PBKDF2 con salt 0x0000000000000000, sha256 y 10000 iteraciones)
	Es decir, queda:
	Key derivada (24 = 3*8 bytes):03db0a157acfe8de523760aa731d8122b25f8d99f3173ec0 (se usará: k1 = 03db0a157acfe8de k2 =523760aa731d8122 k3 = b25f8d99f3173ec0)
	IV derivada (8 bytes): b52849f459a4c20d
	