#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define sideLength 19
#define Height sideLength
#define Width sideLength

int board[Height][Width];//0空，1黑，2白
int gas[Height][Width];//0死，>=1活，-1默认
int checked[Height][Width];
int checkChain[Height][Width];

void ColorStr(const char* content, int color)//输出彩色字符
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	printf("%s", content);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x60);
}
void ColorNumber(int number, int color)//输出彩色数字
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	printf("%d", number);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x60);
}
void gotoxy(short int x, short int y)//以覆写代替清屏，加速Bench
{
	COORD coord = {x, y};
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
void clrscr()//清空屏幕
{
	HANDLE hdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hdout, &csbi);//获取标准输出设备的屏幕缓冲区属性
	DWORD size = csbi.dwSize.X * csbi.dwSize.Y, num = 0;//定义双字节变量
	COORD pos = {0, 0};
	//把窗口缓冲区全部填充为空格并填充为默认颜色
	FillConsoleOutputCharacter(hdout, ' ', size, pos, &num);
	FillConsoleOutputAttribute(hdout, csbi.wAttributes, size, pos, &num);
	SetConsoleCursorPosition(hdout, pos);//光标定位到窗口左上角
}
void setbgcolor(int color)//设置背景颜色
{
	HANDLE hdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hdout, &csbi);
	DWORD size = csbi.dwSize.X * csbi.dwSize.Y, num = 0;
	COORD pos = {0, 0};
	FillConsoleOutputAttribute(hdout, color, size, pos, &num);
	SetConsoleTextAttribute(hdout, color);
}
void showCursor(int visible)//显示或隐藏光标
{
	CONSOLE_CURSOR_INFO cursor_info = {20, visible};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void InitBoard()
{
	int r, c;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			board[r][c] = 0;
			gas[r][c] = -1;
		}
	}
}

void ShowBoard()
{
	int r, c;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			if(board[r][c] == 1)
			{
				ColorStr("●", 0x60);//黑棋
			}
			else if(board[r][c] == 2)
			{
				ColorStr("●", 0x6f);//白棋
			}
			else if(board[r][c] == 0)
			{
				if(r == 0)
				{
					if(c == 0) printf("┌ ");
					else if(c == Width-1) printf("┐ ");
					else printf("┬ ");
				}
				else if(r == Height-1)
				{
					if(c == 0) printf("└ ");
					else if(c == Width-1) printf("┘ ");
					else printf("┴ ");
				}
				else if(c == 0) printf("├ ");
				else if(c == Width-1) printf("┤ ");
				else if((r-3)%6 == 0 && (c-3)%6 == 0)
				{
					printf("╋ ");//星位
				}
				else
				{
					printf("┼ ");
				}
			}
			else
			{
				printf("? ");
			}
		}
		for(c=0; c<Width; c++)
		{
			if(gas[r][c] == -1)
			{
				printf("  ");
			}
			else if(gas[r][c] == 0)
			{
				if(board[r][c] == 1)
				{
					ColorStr(" 0", 0x04);
				}
				else if(board[r][c] == 2)
				{
					ColorStr(" 0", 0xf4);
				}
			}
			else if(gas[r][c] == 1)
			{
				if(board[r][c] == 1)
				{
					ColorStr(" 1", 0x04);
				}
				else if(board[r][c] == 2)
				{
					ColorStr(" 1", 0xf4);
				}
			}
			else if(gas[r][c] < 10)
			{
				if(board[r][c] == 1)
				{
					ColorStr(" ", 0x0a);
					ColorNumber(gas[r][c], 0x0a);
				}
				else if(board[r][c] == 2)
				{
					ColorStr(" ", 0xfa);
					ColorNumber(gas[r][c], 0xfa);
				}
			}
			else
			{
				if(board[r][c] == 1)
				{
					ColorNumber(gas[r][c], 0x0a);
				}
				else if(board[r][c] == 2)
				{
					ColorNumber(gas[r][c], 0xfa);
				}
			}
		}
		printf("\n");
	}
}

int CheckProhibitPoint(int r, int c, int p)//检查某点是否为禁着点
{
	int enemy;
	if(p == 1) enemy = 2;
	else if(p == 2) enemy = 1;
	else return 0;
	// 检查周围4格是否均为敌方棋子
	if(r-1 >= 0 && board[r-1][c] != enemy) return 0;
	if(r+1 < Height && board[r+1][c] != enemy) return 0;
	if(c-1 >= 0 && board[r][c-1] != enemy) return 0;
	if(c+1 < Width && board[r][c+1] != enemy) return 0;
	// 检查周围4个敌方棋子是否仅有1气
	if(r-1 >= 0 && gas[r-1][c] == 1) return 0;
	if(r+1 < Height && gas[r+1][c] == 1) return 0;
	if(c-1 >= 0 && gas[r][c-1] == 1) return 0;
	if(c+1 < Width && gas[r][c+1] == 1) return 0;
	return 1;
}

int CheckEyePoint(int r, int c, int p)//检查某点是否为己方眼位
{
	int check = 1;
	if(board[r][c] != 0) return 0;
	// 检查周围4格是否均为己方棋子
	if(r-1 >= 0 && board[r-1][c] != p) check = 0;
	if(r+1 < Height && board[r+1][c] != p) check = 0;
	if(c-1 >= 0 && board[r][c-1] != p) check = 0;
	if(c+1 < Width && board[r][c+1] != p) check = 0;
	// 检查周围4个己方棋子是否仅有1气
	if(r-1 >= 0 && gas[r-1][c] == 1) check = 0;
	if(r+1 < Height && gas[r+1][c] == 1) check = 0;
	if(c-1 >= 0 && gas[r][c-1] == 1) check = 0;
	if(c+1 < Width && gas[r][c+1] == 1) check = 0;
	return check;
}

int IsAroundCheckChain(int r, int c)
{
	if(r-1 >= 0 && checkChain[r-1][c] == 1) return 1;
	if(r+1 < Height && checkChain[r+1][c] == 1) return 1;
	if(c-1 >= 0 && checkChain[r][c-1] == 1) return 1;
	if(c+1 < Width && checkChain[r][c+1] == 1) return 1;
	return 0;
}

void RefreshGas()
{
	int r, c, r1, c1, p, g;
	int isRising;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			checked[r][c] = 0;
		}
	}
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			if(board[r][c] != 0 && checked[r][c] == 0)
			{
				//标记一块棋
				for(r1=0; r1<Height; r1++)
				{
					for(c1=0; c1<Width; c1++)
					{
						checkChain[r1][c1] = 0;
					}
				}
				p = board[r][c];
				checkChain[r][c] = 1;
				isRising = 1;
				while(isRising == 1)
				{
					isRising = 0;
					for(r1=0; r1<Height; r1++)
					{
						for(c1=0; c1<Width; c1++)
						{
							if(board[r1][c1] == p && checkChain[r1][c1] == 0 && IsAroundCheckChain(r1, c1))
							{
								checkChain[r1][c1] = 1;
								isRising = 1;
							}
						}
					}
				}
				//计算气数
				g = 0;
				for(r1=0; r1<Height; r1++)
				{
					for(c1=0; c1<Width; c1++)
					{
						if(board[r1][c1] == 0 && checkChain[r1][c1] == 0 && IsAroundCheckChain(r1, c1))
						{
							g++;
						}
					}
				}
				for(r1=0; r1<Height; r1++)
				{
					for(c1=0; c1<Width; c1++)
					{
						if(checkChain[r1][c1] == 1)
						{
							gas[r1][c1] = g;
							checked[r1][c1] = 1;
						}
					}
				}
			}
		}
	}
}

void RemoveGo()
{
	int r, c;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			if(gas[r][c] == 0)
			{
				board[r][c] = 0;
				gas[r][c] = -1;
			}
		}
	}
}

int PutGoRandom(int p)
{
	int r, c, check;
	check = 0;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			if(board[r][c] == 0 && CheckProhibitPoint(r, c, p) == 0 && CheckEyePoint(r, c, p) == 0)
			{
				check = 1;
			}
		}
	}
	if(check == 0) return 1;
	while(1)
	{
		r = rand() % Height;
		c = rand() % Width;
		//p = rand() % 2 + 1;
		if(board[r][c] == 0 && CheckProhibitPoint(r, c, p) == 0 && CheckEyePoint(r, c, p) == 0)
		{
			board[r][c] = p;
			RefreshGas();
			RemoveGo();
			board[r][c] = p;
			RefreshGas();
			break;
		}
	}
	return 0;
}

int PutGo(int p)
{
	int r, c;
	HANDLE hdin = GetStdHandle(STD_INPUT_HANDLE);
	COORD mousePos = {0, 0};
	INPUT_RECORD rcd;
	DWORD rcdnum;
	SetConsoleMode(hdin, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	while(1)
	{
		while(1)
		{
			ReadConsoleInput(hdin, &rcd, 1, &rcdnum);
			if(rcd.EventType == MOUSE_EVENT)
			{
				mousePos = rcd.Event.MouseEvent.dwMousePosition;
				if(mousePos.X >= 0 && mousePos.X < 2*Width && mousePos.Y >= 0 && mousePos.Y < Height)
				{
					r = mousePos.Y;
					c = mousePos.X / 2;
					if(rcd.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						break;
					}
				}
				else if(rcd.Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
				{
					return 1;//在场外右击弃权
				}
			}
		}
		if(board[r][c] == 0 && CheckProhibitPoint(r, c, p) == 0)
		{
			board[r][c] = p;
			RefreshGas();
			RemoveGo();
			board[r][c] = p;
			RefreshGas();
			break;
		}
	}
	return 0;
}

void ShowResult()
{
	int rb = 0, rw = 0, r, c;
	for(r=0; r<Height; r++)
	{
		for(c=0; c<Width; c++)
		{
			if(board[r][c] == 1)
			{
				rb++;
			}
			else if(board[r][c] == 2)
			{
				rw++;
			}
			else if(CheckEyePoint(r, c, 1))
			{
				rb++;
			}
			else if(CheckEyePoint(r, c, 2))
			{
				rw++;
			}
		}
	}
	ColorStr("●", 0x60);
	printf(":");
	ColorStr("●", 0x6f);
	printf(" = %d:%d+3.75\n", rb, rw);
}

int main()
{
	int i, p = 1, rb = 0, rw = 0, ab, aw;
	setbgcolor(0x60);
	for(i=0; ; i++)
	{
		clrscr();
		showCursor(0);
		InitBoard();
		srand(i);
		while(1)
		{
			//clrscr();
			gotoxy(0, 0);
			ShowBoard();
			printf("\n当前轮次：");
			if(p == 1)//黑棋轮次
			{
				ColorStr("●", 0x60);
				//getchar();
				rb = PutGoRandom(1);//自动随机落子
				//rb = PutGo(1);//手动鼠标落子
				p = 2;
			}
			else//白棋轮次
			{
				ColorStr("●", 0x6f);
				//getchar();
				rw = PutGoRandom(2);
				//rw = PutGo(2);
				p = 1;
			}
			if(rb == 1 && rw == 1) break;
		}
		printf("\n");
		ShowResult();
		showCursor(1);
		system("pause");
	}
	return 0;
}
