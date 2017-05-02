#include <msp430.h> 
#include <stdlib.h>
# include <time.h>
int OverFlag = 0; 
int Key = 0;
/* 贪吃蛇
 * 
 */
/****************************************************/
/********************** 数据 *************************/
/* 
 * 地图
 * 	0 1 2 3 4 5 6 7
 * 	- - - - - - - -
 * 	0 0 0 0 0 0 0 0 |0
 * 	0 0 0 0 0 0 0 0 |1
 * 	0 0 0 0 0 0 0 0 |2
 * 	0 0 0 0 0 0 0 0 |3
 * 	0 0 0 0 0 0 0 0 |4
 * 	0 0 0 0 0 0 0 0 |5
 * 	0 0 0 0 0 0 0 0 |6
 * 	0 0 0 0 0 0 0 0 |7
 */
#define MapH 8
#define MapL 8
unsigned char Map[8] = {0};


/* 蛇身
 * unsigned long
 *  Snake[20] = 0x00000000
 *
 *     x		  y     direction
	12345678  12345678  12345678  12345678
	--------  --------  --------  --------
	00000000  00000000  00000000  00000000
	00000000  00000000  00000000  00000000
	00000000  00000000  00000000  00000000
    00000010  00100000  00000000  00000000
	.....

	  direction
	  0001 0000  		right
	  0010 0000  		up
	  0100 0000  		down
	  1000 0000  		left
 */
unsigned int SnakeLen  = 1;			//蛇的长度
unsigned long Snake[20] = {0};		
unsigned long temp = 0;				//临时节点信息存储

//获取当前节点的x坐标
int getSnakeX(int index)
{
	int i = 0;
	temp |= Snake[index] >> 24;
	while(!(temp & 0x000000001))
	{
		temp = temp>>1;
		i++;
	}
	temp = 0;
	return (7-i);
}
//获取当前节点的y坐标
int getSnakeY(int index)
{
	int i = 0;
	temp = Snake[index] >> 16;
	while(!(temp & 0x000000001))
	{
		temp = temp>>1;
		i++;
	}
    temp = 0;
	return (7-i);
}



/*
 * 食物
 */
int Foodx = 0;		//食物的x坐标
int Foody = 0;		//食物的Y坐标
void CreateFood()	
{
    int x,y;
    while(1){
        x = (rand()+4) % 8;
        y = (rand()+4) % 8;
        if(Map[y] != (0x80>>x))
        {
            Map[y] |= (0x80>>x);
            Foodx = x;
            Foody = y;
            break;
        }
    }
}

//初始化游戏
void InitMap()
{
    srand(time(0));
    int x,y;
    x = rand()%(MapH);
    y = rand()%(MapL);

    Map[y-1]   |= (0x80>>x-1);
    Snake[0] |= (0x80000000>>(x-1));
    Snake[0] |= (0x00800000>>(y-1));
    Snake[0] |= (0x00000000);
    CreateFood();
}




/*
 * P2.3 DIN
 * P2.4 CS
 * P2.5 CLK  
 */
/****************************************************/
/********************** LED 7219 ********************/
//LED 7219  数据写入
#define din0  P2OUT &= ~BIT3
#define din1  P2OUT |= BIT3
#define load0 P2OUT &= ~BIT4
#define load1 P2OUT |= BIT4
#define clk0  P2OUT &= ~BIT5 
#define clk1  P2OUT |= BIT5
void Write7219(int data)
{
  int i;
  load0;
  for(i=0;i<16;i++)
  {
    clk0;
    if(data&0x8000)
      din1;
    else
      din0;
    data<<=1;
    clk1;
  }
  load1;
}

/* 初始化LED矩阵 */
void InitLed()
{
	P2DIR |=BIT3+BIT4+BIT5;
	Write7219(0x0900);
	Write7219(0x0a08);
	Write7219(0x0b07);
	Write7219(0x0c01);
	Write7219(0x0f00);
    
	Write7219(0x0100);
	Write7219(0x0200);
	Write7219(0x0300);
	Write7219(0x0400);
	Write7219(0x0500);
	Write7219(0x0600);
	Write7219(0x0700);
	Write7219(0x0800);
}

//开场动画
void Animation()
{
    int i,j;
    for(i = 0x0800; i > 0; i-=0x0100)  //row
    {
        for(j = 0; j < 8; j++)         //col
        {
            Write7219(i | (1<<j));
            __delay_cycles(10000);
        }
        Write7219(i);
    }
    __delay_cycles(100000);
    for(i = 0x0100; i < 0x0900; i+=0x0100)  //row
    {
        for(j = 7; j >= 0; j--)             //col
        {
            Write7219(i | (1<<j));
            __delay_cycles(10000);
        }
        Write7219(i);
    }
 
}
/****************************************************/
/********************** 中断 time0 ******************/
//初始化时钟中断time0
void Init_time0()
{
	TACCTL0 |= CCIE;
	TACCR0 = 50000;
	TACTL |= TASSEL_2 + MC_1;	//时钟源、工作模式
}
/****************************************************/
/********************** 中断 Port1 ******************/
//初始化外部中断
void Init_Port1()
{
    P1DIR &=~(BIT2+BIT3+BIT4+BIT5);		 	//输入
    P1DIR |= BIT1;
    P1REN |= BIT2+BIT3+BIT4+BIT5;			//调用内部上（下）拉电阻
    P1OUT |= BIT2+BIT3+BIT4+BIT5;			//决定最终为上拉电阻
    P1IES |= BIT2+BIT3+BIT4+BIT5;			//决定上升沿为中断
    P1IE  |= BIT2+BIT3+BIT4+BIT5;			//开启总中断（I/O中断）
}


//撞墙
void CheckBoard(int x, int y)
{
    if(x<0 || x > 7 || y < 0 || y > 7)
    {
        Animation();
        OverFlag = 1;
    }
}

//撞自己
void CheckBody(int x1, int y1)
{
    int i,x,y;
    for(i = 0; i < SnakeLen; i++)
    {
        x = getSnakeX(i);
        y = getSnakeY(i);
        if(x == x1 && y == y1)
        {
            OverFlag = 1;
            Animation();
        }

    }
}
//刷新地图
void ReLoadMap()
{
    int i,x,y;
    for(i = 0; i < 8; i++) //格式化Map
        Map[i] = 0;
    Map[Foody] |= (0x80>>Foodx);
    for(i = 0; i < SnakeLen; i++)
    {
        x = getSnakeX(i);
        y = getSnakeY(i);
        Map[y] |= (0x80>>x);
    }
}
/****************************************************/
/********************** Main ************************/
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// 关狗
    InitLed();
    Init_time0();
    Init_Port1();
    _EINT();
    Animation();
    //游戏初始化
   InitMap();
   int x,y,i;
    while(!OverFlag){
        x = getSnakeX(0);
        y = getSnakeY(0);
        if(Key != 0){
            if(Key == 2)         //向下移动
            {
            	CheckBoard(x,y+1);
                CheckBody(x,y+1);
                if(x == Foodx && y+1 == Foody)      //前面食物
                {
                    SnakeLen++;
                    CreateFood();
                }
                for(i = SnakeLen-1; i > 0; i--)
                {
                    Snake[i] = Snake[i-1];
                }
				Snake[0] &= ~(0x00800000>>y);       //删除旧蛇头
				Snake[0] |=  (0x00800000>>y+1);     //添加新蛇头
				Snake[0] |=  (0x00000800>>1);
                ReLoadMap();
				Key = 0;
            }
            if(Key == 4)         //向左移动
            { 
                CheckBoard(x-1,y);
                CheckBody(x-1,y);
                if(x-1 == Foodx && y == Foody)      //前面食物
                {
                    SnakeLen++;
                    CreateFood();
                }
                for(i = SnakeLen-1; i > 0; i--)
                {
                    Snake[i] = Snake[i-1];
                }
                Snake[0] &= ~(0x80000000>>x);       //删除旧蛇头
                Snake[0] |=  (0x80000000>>x-1);     //添加新蛇头
                Snake[0] |=  (0x00000800>>0);
                ReLoadMap();
                Key = 0;
            }
            if(Key == 6)         //向右移动
            { 
                CheckBoard(x+1,y);
                CheckBody(x+1,y);
                if(x+1 == Foodx && y == Foody)      //前面食物
                {
                    SnakeLen++;
                    CreateFood();
                }
                for(i = SnakeLen-1; i > 0; i--)
                {
                    Snake[i] = Snake[i-1];
                }
                Snake[0] &= ~(0x80000000>>x);       //删除旧蛇头
                Snake[0] |=  (0x80000000>>x+1);     //添加新蛇头
                Snake[0] |=  (0x00000800>>3);
                ReLoadMap();
                Key = 0;
            }
            if(Key == 8)         //向上移动
            { 
                CheckBoard(x,y-1);
                CheckBody(x,y-1);
                if(x == Foodx && y-1 == Foody)      //前面食物
                {
                    SnakeLen++;
                    CreateFood();
                }
                for(i = SnakeLen-1; i > 0; i--)
                {
                    Snake[i] = Snake[i-1];
                }
                Snake[0] &= ~(0x00800000>>y);       //删除旧蛇头
                Snake[0] |=  (0x00800000>>y-1);     //添加新蛇头
                Snake[0] |=  (0x00000800>>2);
                ReLoadMap();
                Key = 0;
            }
        }
    }
}

//时间中断
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    int i,j=0;
    for(i = 0x0100; i < 0x0900; i+=0x0100)  //row
        Write7219(i | Map[j++]);
}

//按键中断
 #pragma vector = PORT1_VECTOR
 __interrupt void PORT1_ISR(void)
 {
     //上下左右
     //         8(P1.5)
     //4(P1.2)           6(1.4)
     //         2(P1.3)
     unsigned int Push_Key = 0;
     Push_Key = P1IFG&(~P1DIR);
     __delay_cycles(10000);
     if((P1IN&Push_Key) == 0)
     {
         switch(Push_Key)
         {
             case BIT2:
                 Key = 4;
                 break;
             case BIT3:
                 Key = 2;
                 break;
             case BIT4:
                 Key = 6;
                 break;
             case BIT5:
                 Key = 8;
                 break;
             default:
                 break;
         }
     }
     P1IFG = 0;				//清除I/O中断标志位
 }
