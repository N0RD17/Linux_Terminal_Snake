/** includes **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include <termios.h>	// For terminal controls
#include <unistd.h>	// POSIX API



/** Error Handling **/
void die(const char *s)
{
   perror(s);
   exit(1);
}

/** data **/
static struct termios new_termios, original_termios;

enum Direction{
   ARROW_LEFT = 1000,
   ARROW_RIGHT,
   ARROW_UP,
   ARROW_DOWN,
   NONE,
};

int score = 0;

/** TERMINAL STUFF **/
void disableRawMode()
{
   write(STDOUT_FILENO, "\x1b[?25h", 6);
   if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1){
      die("Failed to set termios at old terminal attributes");
   }
   puts("Raw mode disabled!");
}

int read_input()
{
   int nread = 0;
   char c = 0;
   while((nread = read(STDIN_FILENO, &c, 1)) != 1)
   {
      // If failed to read input
      if(nread == -1 && errno != EAGAIN) die("Unable to read input");
   }

   // Escape key
   if (c == '\x1b')
   {
      char seq[3];

      if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
      if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

      if(seq[0] == '[')
      {
         switch(seq[1]){
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
         }
      }

      return '\x1b';
   } else {
      return c;
   }
}

void enableRawMode()
{
   if(tcgetattr(STDIN_FILENO, &original_termios) == -1){
      die("Failed to get old terminal attributes");
   }
   atexit(disableRawMode);

   new_termios = original_termios;

   // Turns off transmission input flag and
   /** Some of these settings below might be turned off on modern terminals **/
   // BRKINT = break condition will be sent to program, like Ctrl-C
   // INPCK enables parity checking, doesnt apply to modern terminal
   // ISTRIP causes 8th bit of each input byte to be stripped, meaning it'll be set to 0. This is probrably already turnedo ff.
   
   new_termios.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON);
   // CS8 is not a flag, its a bit mask with multiple bits,
   // Which sets bitwise-OR(|) operator unlike all the falgs we are turning off.
   //  It sets the size(CS) to 8 bits per byte. On your system it might already be set that way
   new_termios.c_cflag |= (CS8);
   // Turns off local flags like echoing, Canonical, and Signals
   // IEXTEN disables CTRL-V terminal command
   new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
   new_termios.c_cc[VMIN] = 0;
   new_termios.c_cc[VTIME] = 1;

   if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1){
      die("Failed set new terminal attributes");
   }
}

/** Snake Game **/

const int WIDTH = 20;
const int HEIGHT = 10;

struct Point{
   int x, y;
};

struct Snake{
   struct Point head;
   struct Point body[100];
   int length;
   enum Direction direction;
};

struct Point food;
int gameOver = 0;

void initializeSnake(struct Snake* snake)
{
   snake->head.x = WIDTH / 2;
   snake->head.y = HEIGHT / 2;
   snake->length = 1;
   snake->direction = NONE;
}

void initializeFood()
{
   srand(time(NULL));
   food.x = rand() % WIDTH;
   food.y = rand() % HEIGHT;
}

void drawGame(const struct Snake* snake)
{
   // Clear screen
   write(STDOUT_FILENO, "\x1b[2J", 4);

   printf("\x1b[1;1H[Score:%d]\n", score);
   // Sets Cursor Invisible and upper left
   write(STDOUT_FILENO, "\x1b[?25l", 6);
   write(STDOUT_FILENO, "\x1b[2;1H", 6);
   

   for(int i = 0; i < WIDTH + 2; i++){
      printf("\x1b[38;2;85;85;85m#\x1b[m");
   }
   putchar('\n');

   for(int i = 0; i < HEIGHT; i++)
   {
      printf("\x1b[38;2;85;85;85m#\x1b[m");
      for(int j = 0; j < WIDTH; j++)
      {
         if(i == snake->head.y && j == snake->head.x){
            printf("\x1b[38;2;22;198;12m0\x1b[m");
         } else if (i == food.y && j == food.x){
            
            printf("\x1b[38;2;241;76;76mF\x1b[m");	// Food
         } else {
            int isBodypart = 0;
            for(int k = 0; k < snake->length; k++)
            {
               if(i == snake->body[k].y && j == snake->body[k].x){
                  isBodypart = 1;
                  break;
               }
            }
            if(isBodypart){
               printf("\x1b[38;2;0;128;0mo\x1b[m");
            } else {
               putchar(' ');
            }
         }
      }
      printf("\x1b[38;2;85;85;85m#\x1b[m");
      putchar('\n');
   }

   for(int i = 0; i < WIDTH + 2; i++){
      printf("\x1b[38;2;85;85;85m#\x1b[m");
   }
   putchar('\n');
   fflush(stdout);
}

void getInput(struct Snake* snake)
{
   int c = read_input();
   switch(c)
   {
      case ARROW_UP:{
         snake->direction = ARROW_UP;
      }break;

      case 'w':{
         snake->direction = ARROW_UP;
      }break;

      case ARROW_DOWN:{
         snake->direction = ARROW_DOWN;
      }break;

      case 's':{
         snake->direction = ARROW_DOWN;
      }break;

      case ARROW_LEFT:{
         snake->direction = ARROW_LEFT;
      }break;

      case 'a':{
         snake->direction = ARROW_LEFT;
      }break;

      case ARROW_RIGHT:{
         snake->direction = ARROW_RIGHT;
      }break;

      case 'd':{
         snake->direction = ARROW_RIGHT;
      }break;

      default:{
         snake->direction = NONE;
      }break;

      case 'q':{
         gameOver = 1;
      }break;
   }
}

void updateGame(struct Snake* snake)
{
   struct Point newHead = snake->head;

   switch (snake->direction)
   {
      case ARROW_UP:{
         newHead.y--;
      }break;

      case ARROW_DOWN:{
         newHead.y++;
      }break;

      case ARROW_LEFT:{
         newHead.x--;
      }break;

      case ARROW_RIGHT:{
         newHead.x++;
      }break;

      case NONE:{
         newHead.x += 0;
         newHead.y += 0;
      }break;

      default:{
      }break;
   }

   // Check for collisions
   if(newHead.x < 0 || newHead.x >= WIDTH || newHead.y < 0 || newHead.y >= HEIGHT){
      gameOver = 1; // Hit wall
   }

   // Check if snake ate food
   if(newHead.x == food.x && newHead.y == food.y){
      score++;
      initializeFood();
      snake->length++;
   }

   // Move the snake
   for(int i = snake->length - 1; i > 0; i--){
      if(snake->direction != NONE){
         snake->body[i] = snake->body[i - 1];
      }
   }

   snake->body[0] = newHead;

   // Check if snake collided with itself
   for(int i = 1; i < snake->length; i++)
   {
      if(newHead.x == snake->body[i].x && newHead.y == snake->body[i].y){
         gameOver = 1;
      }
   }

   // Update the head
   snake->head = newHead;
}


int main(void)
{
   enableRawMode();
   struct Snake snake;
   initializeSnake(&snake);
   initializeFood();

   while(!gameOver)
   {
      drawGame(&snake);

      getInput(&snake);
      updateGame(&snake);
   }

   puts("Game Over!\n");

   return 0;
}
