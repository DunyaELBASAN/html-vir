/*
	Very sample ROT13 IDC Script for IDA Pro.
        Created for malware analysis.
        Select the encoded strings and press Shift+F2. just enter "rot13();" without the quotes
        Script by Nicolas Brulez.
*/

#include <idc.idc>
static rot13(void)
{
 auto counter,J,len,end,address; 	

     address = SelStart();
     end=SelEnd();
     len=end-address;
     Message("ROT13: Converting Strings...\n");

    for(counter=0;counter<len;counter++) 
    { 
     J = Byte(address); 
     if (J >= 65 && J<=90) {

		J = ((J-52) % 26) + 65;
	}

     if (J >= 97 && J<=122) {

		J = ((J-84) % 26) + 97;
	}

     PatchByte(address,J); 
     address++;
    } 
     Message("ROT13: Job Complete!\n");
}




