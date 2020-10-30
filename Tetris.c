#include <C8051F020.h>
#include "lcd.h"
#include "portmap.h"

unsigned char fall_speed = 0;

bit collide;
bit end_game = 0;
bit rotate_pot = 0;
bit drop_btn;
unsigned char rotate_pot_value = 0;
unsigned char move_pot_value = 0;
int led;
char count = 0;
char square_y;
char square_x = 8;
char rotation = 0;
char position = 8;
char next_pos, next_rot;
unsigned char block_number = 0;
unsigned char next_block =0;
unsigned int score = 0;
unsigned char random = 0;

code char score_message[]= "SCORE";

char Sqx[4],Sqy[4];

code char sine[] = { 48, 89, 116, 126, 116, 89, 48, 0, -48, -89, -116, -126, -116, -89, -48, 0 };

xdata unsigned int play_area[20];
char phase = sizeof(sine)-1;	// current point in sine to output

unsigned int duration = 0;		// number of cycles left to output
unsigned int start = 0;
int env = 32767;
unsigned char decay = 0;
bit flag = 0;


code unsigned char mask[] = {0xf,0xf0,0x9,0x90,0x9,0x90,0xf,0xf0};

code char blocks_x[7][4] = {{0,0,0,0},
							{0,-1,-1,1},
							{-1,-1,0,1},
							{0,0,1,1},
							{0,0,1,1},
							{-1,0,0,1},
							{0,0,1,1}};

code char blocks_y[7][4] = {{0,1,2,-1},
							{0,0,-1,0},
							{1,0,0,0},
							{0,1,1,0},
							{-1,0,0,1},
							{0,1,0,0},
							{1,0,0,-1}};



code unsigned char move_minus[] = {0,11,24,37,49,62,75,88,101,113};
code unsigned char move_plus[] =  {15,27,40,53,66,79,91,104,117,127};

code unsigned char rotate_plus[] =  {0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,0x92,0xA2,0xB2,0xC2,0xD2,0xE2,0xF2,0xFF};
code unsigned char rotate_minus[] = {0x00,0x0E,0x1E,0x2E,0x3E,0x4E,0x5E,0x6E,0x7E,0x8E,0x9E,0xAE,0xBE,0xCE,0xDE,0xEE};

code unsigned char end_message[] = "Game over";

void ADC0_ISR(void) interrupt 15
{
	if(rotate_pot)
	{
		rotate_pot_value = ADC0H;

		if(rotate_pot_value < rotate_minus[rotation])
		{
				next_rot = rotation - 1;
		}
		else if(rotate_pot_value > rotate_plus[rotation])
		{
				next_rot = rotation +1;
		}
		else
			next_rot = rotation;

		rotate_pot = 0;
		AMX0SL = 0x00;	//Read from AIN0
	}
	else
	{
		move_pot_value = ADC0H;


		move_pot_value = move_pot_value >>1;

		if(move_pot_value < move_minus[position])
		{
				next_pos = position -1;
		}
		else if(move_pot_value > move_plus[position])
		{
				next_pos = position + 1;
		}
		else
			next_pos = position;


		rotate_pot = 1;

		AMX0SL = 0x01;	//Read from AIN1

	}

	random = random << 1;
	if(ADC0L & 0x10)	//Since ADC0 is left justified, 		
		random |= 1;	//ADC0L bit 4 is the least significant bit.

	AD0INT = 0;

}


void init_adc0(void) 
{
	ADC0CN = 0x8D;	//On, started on T2, right justified
	REF0CN = 0x07;	
	AMX0CF = 0x00;
	AMX0SL = 0x00;	//Read from ADC0
}

void timer4(void) interrupt 16
{
	T4CON = 4;

	DAC0H = ((sine[phase] * (env>>8))>>7) +128;
	

	if ( phase < sizeof(sine)-1 )	// if mid-cycle
	{				// complete it
		phase++;
	}
	else if ( duration > 0 )	// if more cycles left to go
	{				// start a new cycle
		phase = 0;
		if(env > decay)
		{
			env -= decay;
		}
		duration--;
	}

}

void play_note(int dur, dec, reload)
{
	RCAP4H = reload >> 8;
	RCAP4L = reload;				
	env = 32767;
	decay = dec;
	duration = dur;

}

void draw_square(char col, char y)
{
	int i; char j;
	if ( y < 0 ) return;
	i = (col>>1) * 128 + y;
	for ( j = col & 1; j < 8; j+=2 )
	{
	 if ( screen[i] & mask[j] ) collide = 1;
	 screen[i] |= mask[j];
	 i++;
	}
}

void draw_play_area()
{
   char i,j,y;
   for (i=0; i<84; i+=4 )
   {
     draw_square(2,i);
	 draw_square(13,i);
   }
   for (i = 3; i<13; i++ )
   {
     draw_square(i,0);
   }
	y = 0;
   for(i = 0;i<20; i++)
   {
   		y+=4;

		for(j = 0; j<10; j++)
		{
			if((play_area[i]>>j) & 1)
			{
				draw_square(j+3,y);
			}
		}
   }

   for(i = 0; i < 7; i++)
   {
		draw_square(i,124);
		draw_square(i,100);
   }
   for(i = 104; i<124; i+=4)
   {
		draw_square(0,i);
		draw_square(6,i);
   }
}


void position_block(char sqx[4],char sqy[4], block,turn,x,y)
{
	if(turn == 0)
	{
		sqx[0] = x + blocks_x[block][0];
		sqy[0] = y + blocks_y[block][0];
		sqx[1] = x + blocks_x[block][1];
		sqy[1] = y + blocks_y[block][1];
		sqx[2] = x + blocks_x[block][2];
		sqy[2] = y + blocks_y[block][2];
		sqx[3] = x + blocks_x[block][3];
		sqy[3] = y + blocks_y[block][3];
	}

	else if(turn == 1)
	{
		sqx[0] = x - blocks_y[block][0];
		sqy[0] = y + blocks_x[block][0];
		sqx[1] = x - blocks_y[block][1];
		sqy[1] = y + blocks_x[block][1];
		sqx[2] = x - blocks_y[block][2];
		sqy[2] = y + blocks_x[block][2];
		sqx[3] = x - blocks_y[block][3];
		sqy[3] = y + blocks_x[block][3];
	}

	else if(turn == 2)
	{
		sqx[0] = x - blocks_x[block][0];
		sqy[0] = y - blocks_y[block][0];
		sqx[1] = x - blocks_x[block][1];
		sqy[1] = y - blocks_y[block][1];
		sqx[2] = x - blocks_x[block][2];
		sqy[2] = y - blocks_y[block][2];
		sqx[3] = x - blocks_x[block][3];
		sqy[3] = y - blocks_y[block][3];
	}

	else if(turn == 3)
	{
		sqx[0] = x + blocks_y[block][0];
		sqy[0] = y - blocks_x[block][0];
		sqx[1] = x + blocks_y[block][1];
		sqy[1] = y - blocks_x[block][1];
		sqx[2] = x + blocks_y[block][2];
		sqy[2] = y - blocks_x[block][2];
		sqx[3] = x + blocks_y[block][3];
		sqy[3] = y - blocks_x[block][3];
	}
}

bit check_x(char sqx[4],char sqy[4],block,turn,x,y)
{
	bit valid_x;
	
	valid_x = 0;
	position_block(sqx,sqy,block,turn,x,y);
	
	valid_x |= play_area[sqy[0]] & 1<<sqx[0];
	valid_x |= play_area[sqy[1]] & 1<<sqx[1];
	valid_x |= play_area[sqy[2]] & 1<<sqx[2];
	valid_x |= play_area[sqy[3]] & 1<<sqx[3];

	valid_x |= sqx[0] & 0xFC00;
	valid_x |= sqx[1] & 0xFC00;
	valid_x |= sqx[2] & 0xFC00;
	valid_x |= sqx[3] & 0xFC00;

	valid_x |= (1<<sqx[0]) & 0xFC00;
	valid_x |= (1<<sqx[1]) & 0xFC00;
	valid_x |= (1<<sqx[2]) & 0xFC00;
	valid_x |= (1<<sqx[3]) & 0xFC00;

	if(sqy[0] < 0 || sqy[1] < 0 || sqy[2] < 0 || sqy[3] < 0)
		valid_x = 1;

	return ~valid_x;
}

void adjust_x(char sqx[4],char *x)
{
	while((sqx[0] > 9) || (sqx[1] > 9) || (sqx[2] > 9) || (sqx[3] > 9))
	{
		sqx[0] --;
		sqx[1] --;
		sqx[2] --;
		sqx[3] --;
		*x -= 1;
	}
	while((sqx[0] < 0) || (sqx[1] < 0) || (sqx[2] < 0) || (sqx[3] < 0))
	{
		sqx[0] ++;
		sqx[1] ++;
		sqx[2] ++;
		sqx[3] ++;
		*x += 1;
	}

}

void adjust_y(char sqy[4],char *Y)
{
	while((sqy[0] > 19) || (sqy[1] > 19) || (sqy[2] > 19) || (sqy[3] > 19))
	{
		sqy[0] --;
		sqy[1] --;
		sqy[2] --;
		sqy[3] --;
		*Y -=4;
	}
}


void draw_falling_block(char block)
{
	char turn, next_turn;
	char part_y;

	char sqx[4],sqy[4];
	char block_y,block_y2;

	char i;

	turn = rotation % 4;
	next_turn = next_rot % 4;
	block = block % 7;


	block_y = (square_y)/4;
	block_y2 = (square_y +3)/4;
	
	if(square_y < 0)
		block_y = -1;

	if(check_x(Sqx,Sqy,block,turn,position,block_y))
	{
		if(position != next_pos)
		{
			if(check_x(sqx,sqy,block,turn,next_pos,block_y2)&&
				check_x(sqx,sqy,block,turn,next_pos,block_y))
			{
				position = next_pos;

				for(i = 0; i<4; i++)
				{
					Sqx[i] = sqx[i];
					Sqy[i] = sqy[i];
				}
			}
			else
			{
				for(i = 0; i<4; i++)
				{
					sqx[i] = Sqx[i];
					sqy[i] = Sqy[i];
				}
			}
		}
	


		if(next_turn != turn)
		{
			if(check_x(sqx,sqy,block,next_turn,position,block_y2)&&
				check_x(sqx,sqy,block,next_turn,position,block_y))
			{
				rotation = next_rot;
			
			}
			else if(check_x(sqx,sqy,block,next_turn,position+1,block_y2)&&
					check_x(sqx,sqy,block,next_turn,position+1,block_y))
			{
				position ++;
				rotation = next_rot;
				turn = next_turn;
			}
			else if(check_x(sqx,sqy,block,next_turn,position-1,block_y2)&&
					check_x(sqx,sqy,block,next_turn,position-1,block_y))
			{
				position --;
				rotation = next_rot;
				turn = next_turn;
			}
	/*		else if(check_x(sqx,sqy,block,next_turn,position+2,block_y))
			{
				position +=2;
				rotation = next_rot;
				turn = next_turn;
			}
			else if(check_x(sqx,sqy,block,next_turn,position-2,block_y))
			{
				position -=2;
				rotation = next_rot;
				turn = next_turn;
			}
	*/		else
			{
				for(i = 0; i<4; i++)
				{
					sqx[i] = Sqx[i];
					sqy[i] = Sqy[i];
				}
			}
		
		
		
			for(i = 0; i<4; i++)
			{
				Sqx[i] = sqx[i];
				Sqy[i] = sqy[i];
			}

		}
	}


	part_y = (square_y - 4*block_y) + 4;

//	if(square_y <0)
//		part_y = 3;

	draw_square(Sqx[0]+3,4*Sqy[0] + part_y);
	draw_square(Sqx[1]+3,4*Sqy[1] + part_y);
	draw_square(Sqx[2]+3,4*Sqy[2] + part_y);
	draw_square(Sqx[3]+3,4*Sqy[3] + part_y);

}

void draw_next_block()
{
	char sqx[4], sqy[4];
	position_block(sqx,sqy,next_block,rotation%4,3,28);
	
	draw_square(sqx[0],4*sqy[0]);
	draw_square(sqx[1],4*sqy[1]);
	draw_square(sqx[2],4*sqy[2]);
	draw_square(sqx[3],4*sqy[3]);
}

void add_block(char sqx[4],char sqy[4])
{
	char i,j = 0;

	play_area[sqy[0]] |= 1<<sqx[0];
	play_area[sqy[1]] |= 1<<sqx[1];
	play_area[sqy[2]] |= 1<<sqx[2];
	play_area[sqy[3]] |= 1<<sqx[3];

	for(i = 0; i < 20; i++)
	{
		if(play_area[i] == 0x03FF)
		{
			for(j=i;j <19;j++)
			{
				play_area[j] = play_area[j+1];
			}
			score ++;
			play_area[19] = 0;
			i --;
			flag = 1;
		}

	}
	if(flag)
	{
		play_note(349, 94, 0xF843);
		flag = 0;
	}
	else
		play_note(262, 125, 0xF5AD);
}	

// x and y in pixels
void disp_ch(unsigned char x, 
		unsigned char y, unsigned char ch)
{
	int ifont, iscr;

	char k,l;
	unsigned char row,shift;

	if(ch < ' ')
		return;
	ifont = (ch - ' ') *5;
	iscr = ((x & 0xF8)<<4) +y -1;
	shift = x & 0x7;	//x%8
	for(k=0; k < 8; k++)
	{
		row = 0;
		for(l=4;l>-1;l--)
		{	
			row = row <<1;
			row |= (font5x8[ifont+l]>>(7-k)) & 0x01;
			//row |= (font5x8[ifont+l]) & 0x80;
			
		}
		//row = row >> 2;

		screen[iscr +k] |= row <<shift;
		screen[iscr +128+k] |= row >> (8-shift);
	}
}
void disp_score()
{
	char num,j,k;
	unsigned int remain;
	k = 57;
	remain = score;
	for(j = 0; j<4;j++)
	{
		num = remain%10; 
		remain /=10;
		disp_ch(k,85,num +0x30);
		k -= 6;
	}
	disp_ch(k,85,remain +0x30);
	k -= 8;
	for(j = 4;  j >= 0; j--)
	{
		disp_ch(k,85,score_message[j]);
		k-= 6;
	}

}
void main()
{
	int k;
	char i,j,part_y;
	WDTCN = 0xde;  // disable watchdog
	WDTCN = 0xad;
	XBR2 = 0x40;   // enable port output
	XBR0 = 4;      // enable uart 0
	OSCXCN = 0x67; // turn on external crystal
	TMOD = 0x20;   // wait 1ms using T1 mode 2
	TH1 = -167;    // 2MHz clock, 167 counts - 1ms
	TR1 = 1;
	while ( TF1 == 0 ) { }          // wait 1ms
	while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
	OSCICN = 8;    // switch over to 22.1184MHz
	init_lcd();
	init_portmap();
	init_adc0();
	RCAP2H = 0;
	RCAP2L = 0;
	T2CON = 4; // start timer
	IE = 0x80;
	P2 = 0xFF;
	DAC0L = 0;
	REF0CN = 3;	// turn on voltage reference
	DAC0CN = 0x84; 	// update on DAC0H, left justified
	CKCON = 0x40;	//T4 uses system clock.
	EIE2 = 0x06;
	T4CON = 4;	// timer 4, auto reload

	for(i = 0; i<20;i++)
	{
		play_area[i] = 0;
	}

	draw_play_area();
	disp_score();

	refresh_screen();


   for(; ;)
   {

		while(!((~get_buttons())&0x02)) 	//Wait until the start button is pressed
		{
			position = next_pos;
			rotation = next_rot;
			block_number = random% 7;
			next_block = (random + 2) % 7;
		}

		square_y = 88;
		
		position_block(Sqx,Sqy,block_number,rotation%4,position,square_y/4);
		adjust_y(Sqy,&square_y);
		adjust_x(Sqx,&position);
		
		for(i = 0; i<20;i++)
		{
			play_area[i] = 0;
		}

		end_game = 0;
		score = 0;
		drop_btn = 0;


		collide = 0;
		
		while (!end_game)
		{
			blank_screen();

		//			clear_leds();
		//			switches = ~get_switches();
		//			butt = (~get_buttons())&0x03;
		//		 set_leds(switches + ((butt)<<8));

		// handle movement here
			if (collide)
			{
				square_y ++;
				position_block(Sqx,Sqy,block_number,rotation%4,position,square_y/4);
				add_block(Sqx,Sqy);

				square_y = 88;
				collide = 0;
				
				block_number = next_block;
				next_block = random % 7;

				position_block(Sqx,Sqy,block_number,rotation%4,position,square_y/4);
				adjust_y(Sqy,&square_y);
				adjust_x(Sqx,&position);

				part_y = (square_y % 4) + 4;

				draw_square(Sqx[0]+3,4*Sqy[0] + part_y);
				draw_square(Sqx[1]+3,4*Sqy[1] + part_y);
				draw_square(Sqx[2]+3,4*Sqy[2] + part_y);
				draw_square(Sqx[3]+3,4*Sqy[3] + part_y);
				
				draw_play_area();	

				blank_screen();
/*				draw_play_area();
				draw_falling_block(block_number);

*/				if(collide)
					end_game = 1;

				collide = 0;
				j = 0;

			}
			else if(((~get_buttons())&0x01) && ~drop_btn)
			{
				drop_btn = 1;

				while(check_x(Sqx,Sqy,block_number,rotation%4,position,square_y/4))
				{
					square_y -= 1;
				}

			}
			else 
			{
				if(((~get_buttons())&0x01) == 0)
				{
					drop_btn = 0;
				}

				fall_speed = ~get_switches();
				j++;
				if(j> fall_speed)
				{
					square_y -= 1; 
					j=0;
				}


			}



			// draw stuff here
			draw_play_area();
			refresh_screen();
			collide = 0;
			draw_falling_block(block_number);
			refresh_screen();
			draw_next_block();
			disp_score();
			refresh_screen();

			// wait for T2 overflow to refresh screen
			while ( TF2 == 0 ) { }
			TF2 = 0;
			refresh_screen();

			// turn on an LED if there is a collision
			clear_leds();
			led = 1&drop_btn;
			led = led << 9;
			set_leds(led);
			if ( collide ) set_leds(1);
		}



		k = 42;
		for (j = 0; j < 8; j++)
		{
			for(i = 0; i < 14; i++)
				screen[k+i] = 0;
			k+=128;

		}

		j = 5;
		for(i = 0; i < 9; i++)
		{
			disp_ch(j,45,end_message[i]);
			j += 6;
		}
		refresh_screen();
   }
}