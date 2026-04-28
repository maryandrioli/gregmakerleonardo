#include "Arduino.h"

/*
//////////////////////////////////
// MAPEAMENTO DE TECLAS DO GREG //
//////////////////////////////////
  
  - edite o vetor keyCodes abaixo para mudificar as teclas que o GREG envia para cada botao
  - os comentarios a direita indicam a qual botao o comando esta sendo atribuido.
    NAO ALTERE O COMENTÁRIO, SOMENTE O COMANDO
  - altere a tecla substituindo o comando. trocando KEY_UP_ARROW por '#' a seta para cima
    passará a inserir o caractere #
  - podem ser utilizados alem de caracteres outras teclas de funcao, listadas no fim do arquivo

*/

int keyCodes[NUM_INPUTS] = {
  KEY_UP_ARROW,       // Seta_cima // Pino 12
  KEY_DOWN_ARROW,     // Seta_baixo // Pino 8
  KEY_LEFT_ARROW,     // Seta_esquerda // Pino 13
  KEY_RIGHT_ARROW,    // Seta_direita // Pino 7
  ' ',                // SPACE // Pino 5
  KEY_RETURN,         // ENTER // Pino 6
  
  'f',                // Header1(f) // Pino 4
  'd',                // Header2(d) // Pino 3
  's',                // Header3(s) // Pino 2
  'a',                // Header4(a) // Pino 1
  'w',                // Header5(w) // Pino 0
  
  MOUSE_MOVE_UP,      // MOUSE_cima // Pino 23 (A5)
  MOUSE_MOVE_DOWN,    // MOUSE_baixo // Pino 22 (A4)
  MOUSE_MOVE_LEFT,    // MOUSE_esquerda // Pino 21 (A1)
  MOUSE_MOVE_RIGHT,   // MOUSE_direita // Pino 20 (A0)
  MOUSE_RIGHT,        // MOUSE_clique_direito // Pino 19 (A2)
  MOUSE_LEFT          // MOUSE_clique_esquerdo // Pino 18 (A3)
};


/*

///////////////////////
// NOMES DAS TECLAS ///
///////////////////////

- Teclas de função que podem ser utilizadas acima

KEY_LEFT_CTRL
KEY_LEFT_SHIFT		
KEY_LEFT_ALT		
KEY_LEFT_GUI		
KEY_RIGHT_CTRL		
KEY_RIGHT_SHIFT		
KEY_RIGHT_ALT	
KEY_RIGHT_GUI

KEY_UP_ARROW,
KEY_DOWN_ARROW,
KEY_LEFT_ARROW,
KEY_RIGHT_ARROW,

KEY_BACKSPACE		
KEY_TAB				
KEY_RETURN			
KEY_ESC				
KEY_INSERT			
KEY_DELETE			
KEY_PAGE_UP			
KEY_PAGE_DOWN		
KEY_HOME
KEY_END				
KEY_CAPS_LOCK	

MOUSE_MOVE_UP,      
MOUSE_MOVE_DOWN,    
MOUSE_MOVE_LEFT,    
MOUSE_MOVE_RIGHT,   
MOUSE_RIGHT,        
MOUSE_LEFT          

KEY_F1				
KEY_F2				
KEY_F3				
KEY_F4				
KEY_F5				
KEY_F6				
KEY_F7				
KEY_F8				
KEY_F9				
KEY_F10
KEY_F11				
KEY_F12			

*/
