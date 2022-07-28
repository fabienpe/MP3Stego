MP3Stego
--------

Fabien A.P. Petitcolas, Computer Laboratory, Cambridge
5 August 1998


When looking at the steganographic tools available on the Net, it occurred to me
that nothing had been done to hide information in MP3 files, that is sound 
tracks compressed using the MPEG Audio Layer III format. There is a growing 
interest world-wide in MP3 files because they offer near-CD quality at 
compression ratio of 11 to 1 (128 kilobits per second). This gives a very good 
opportunity for information hiding. 

MP3Stego will hide information in MP3 files during the compression process. The 
data is first compressed, encrypted and then data hidden in the MP3 bit stream.
Although MP3Stego has been written with steganographic applications in mind it
might be used as a watermarking system for MP3 files. Any opponent can 
uncompress the bit stream and recompress it; this will delete the hidden
information -- actually this is the only attack we know yet -- but at the 
expense of severe quality loss.

The hiding process takes place at the heart of the Layer III encoding process 
namely in the inner_loop. The inner loop quantizes the input data and increases 
the quantizer step size until the quantized data can be coded with the available
number of bits. Another loop checks that the distortions introduced by the 
quantization do not exceed the threshold defined by the psycho acoustic model. 
The part2_3_length variable contains the number of main_data bits used for 
scalefactors and Huffman code data in the MP3 bit stream. We encode the bits 
as its parity by changing the end loop condition of the inner loop. Only 
randomly chosen part2_3_length values are modified; the selection is done 
using a pseudo random bit generator based on SHA-1. 

We have discussed earlier the power of parity for information hiding [1]. 
MP3Stego is a practical example of it. There is still space for improvement 
but I thought that some people might be interested to have a look at it. 

The full C code and binaries are available from:

<http://www.cl.cam.ac.uk/~fapp2/steganography/mp3stego/>

Usage exemple: 
  encode -E data.txt sound.wav sound.mp3 
    compresses sound.wav and hides data.txt. This produces the output called 
    sound.mp3 
  decode -X sound.mp3 
    uncompresses sound.mp3 into sound.mp3.pcm and attempt to extract hidden 
    information. The hidden message is decrypted, uncompress and saved into 
    sound.mp3.txt. 

Don't forget to let me know your suggestions and comments: w+fabien22@petitcolas.net 

This computer program is based on: 
- 8hz-mp3 0.2b -- 8Hz implementation of MP3 encoder; 
- MP3 Decoder (dist10) of the ISO MPEG Audio Subgroup Software Simulation Group; 
- ZLib 1.1.3 compression library by Jean-Loup Gailly’s ZLib; 
- Eric’s Young implementation of 3DES; 
- James J. Gillogly’s implementation of SHA-1; 
- ISO/IEC 11172-3:1993, Information technology -- Coding of moving pictures 
  and associated audio for digital storage media at up to about 1,5 Mbit/s -- 
  Part 3: Audio, with the permission of ISO. Copies of this standards can be 
  purchased from the British Standards Institution, 389 Chiswick High Road, 
  GB-London W4 4AL, Telephone:+ 44 181 996 90 00, Telefax:+ 44 181 996 74 00 
  or from ISO, postal box 56, CH-1211 Geneva 20, Telephone +41 22 749 0111, 
  Telefax +4122 734 1079. Copyright remains with ISO. 

[1] Ross J. Anderson and Fabien A.P. Petitcolas. On The Limits of 
    Steganography. IEEE Journal of Selected Areas in Communications, 
    16(4):474-481, May 1998. Special Issue on Copyright & Privacy Protection. 
    ISSN 0733-8716. 


History
-------

5 August 1998 -    MP3Stego is advertised on both the steganography and
                   watermarking mailing lists.

11 February 1999 - MP3Stego now inform users if the data to be hidden is to
                   big for the cover-sound.
