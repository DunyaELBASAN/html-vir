

void main(void){

	//we will change the byte values of these 2 variables dynamically
	//so this will help us locate them in the asm code
	int i;
	char b;
	
	char *buffer = 4194304; //imagebase (400000)
	long length =  48879; // \xBEEF  <-length of code

    
	buffer += 57005; // \xDEAD  <-pointer to OEP ( + offset)


	for(i=0; i < length; i++){
		
		b = buffer[i];
		b = b ^ 15;
		buffer[i] = b;  

	}

	_asm jmp buffer



}