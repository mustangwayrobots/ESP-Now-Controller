// display message on screen
void displayMsg( const char *NewMessage){
  static char currentMessage[50];
  char *orignal;
  char *token, *theRest;
  String myLine[5];
  
  if(strcmp(currentMessage, NewMessage) != 0){
    strcpy( currentMessage, NewMessage );
    if (strlen(currentMessage) <= 9){
      u8g2.clearBuffer();          
      u8g2.setFont(u8g2_font_inb16_mr); 
      u8g2.drawStr(0, 24, currentMessage); 
      u8g2.sendBuffer();          

    }else{
      int count = 0;
      orignal = strdup(NewMessage);
      theRest = orignal;
      myLine[count] = "";
      while((token = strtok_r(theRest, " ", &theRest))){
        if (myLine[count].length() + strlen(token) < 9){
          if (myLine[count].length() > 0){
            myLine[count] += " ";          
          }
          myLine[count] += token;
        }else{
          count++;
          myLine[count] = String(token);          
        }
      } 
      u8g2.clearBuffer();          
      u8g2.setFont(u8g2_font_luBS14_tf);      // Restored original large font
      for(int i = 0; i <= count; ++i){
        int str_len = myLine[i].length() + 1;
        char char_array[str_len];
        myLine[i].toCharArray(char_array, str_len);        
        u8g2.drawStr(0, ((i+1)*16)-2, char_array); 
      }
      u8g2.sendBuffer();          

      free(orignal); // Prevents heap memory leaks
    }   
  }
}