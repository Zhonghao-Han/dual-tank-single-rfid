/************************************************************
 Shanghai Fudan Microelectronics Group Company Limited
 
 file name:FM11_demo.c
 
 version:v0.10

 project:YX1323
*************************************************************/


#include "FM11_Demo.h"

uint8_t FlagFirstFrame = OFF;	//卡片首帧标识
uint32_t FSDI = 64-2;	//-4帧长度PCD
uint8_t irq_data_in = 0;	//非接数据接收终端标识
uint8_t irq_rxdone = 0;
uint8_t irq_txdone = 0;
uint8_t rfLen;
uint8_t rfBuf[255];
uint8_t crc_err = 0;
uint8_t FM441_AUTH_0= 0xff;//需要验证的起始页地址
uint8_t FM441_AUTH_Key[10]= {0x11,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//10字节密码
uint8_t fm365SakConfig =0x20;
uint8_t	t2tAppBuffer[16+2040+12] = {
    0x00, 0x00, 0x00, 0x00, // uid read from eeprom at pwron_int
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
	  0xe1, 0x10, 0x6D, 0x00, 
    0x03, 0x00, 0xfe, 0x00,
};

/*********************************************************************************************************
** 函数名称:	FM11_RFDataTx
** 函数功能:	RF数据回发
** 输入参数:    ilen:回发数据长度
** 					*ibuf:回发的数据
** 输出参数:    无
** 返回值:      无
*********************************************************************************************************/
void FM11_RFDataTx(uint32_t ilen,uint8_t *ibuf)
{
	uint32_t slen;
	uint8_t *sbuf;
	
	slen = ilen;
	sbuf = ibuf;

	if(slen <= 32)
	{
		FM11_WriteFIFO(sbuf,slen);		//write fifo	有多少发多少
		slen = 0;
	}
	else
	{
		FM11_WriteFIFO(sbuf,32);			//write fifo	先发32字节进fifo
		
		slen -= 32;		//待发长度－32
		sbuf += 32;		//待发数据指针+32
	}
	
	FM11_WriteReg(RF_TXEN_REG,0x55);	//写0x55时触发非接触口回发数据

	while(slen>0)
	{		
		if((FM11_ReadReg(FIFO_WORDCNT_REG) & 0x3F )<=8)
		{
			if(slen<=24)
			{
				FM11_WriteFIFO(sbuf,slen);			//write fifo	先发32字节进fifo
				slen = 0;				
			}
			else
			{
				FM11_WriteFIFO(sbuf,24);			//write fifo	先发32字节进fifo
				
				slen -= 24; 	//待发长度－24
				sbuf += 24; 	//待发数据指针+24
			}
		}
		
		FM11_WriteReg(RF_TXEN_REG,0x55);	//写0x55时触发非接触口回发数据
	}

	irq_txdone = 0;
}

/*********************************************************************************************************
** 函数名称:	FM11_RFDataRx  中断方式，需要连接IRQ
** 函数功能:	写FIFO
** 输入参数:    rbuf:读取数据
** 输出参数:    无
** 返回值:      读取的数据长度
*********************************************************************************************************/
uint32_t FM11_RFDataRx(uint8_t *rbuf)
{
  uint32_t rlen,temp;
	rlen = 0;
	temp = 0;
	do
	{	
		if((FM11_ReadReg(FIFO_WORDCNT_REG) & 0x3F )>=24)	//查fifo是否到24字节
		{
			FM11_ReadFIFO(24,&rbuf[rlen]);		//渐满之后读取24字节			
			rlen += 24;
		}
		if( ( (FM11_ReadReg(FIFO_WORDCNT_REG) & 0x3F ) ==0 )&&(rlen==0) && (irq_rxdone == 0))		
		{				
			return 0;
		}
	}while(irq_rxdone == 0);
	
	irq_rxdone = 0;

	temp =(uint32_t)( FM11_ReadReg(FIFO_WORDCNT_REG) & 0x3F);	//接收完全之后，查fifo有多少字节

	FM11_ReadFIFO(temp,&rbuf[rlen]);		//读最后的数据
	rlen += temp;
	
#if 0
	printf("temp %02x \r\n",temp);
	printf("rlen %02x \r\n",rlen);
#endif

	if(rlen <= 2)	
		return 0;
	rlen -= 2;	//2字节crc校验
	return rlen;
}


// /*********************************************************************************************************
// ** 函数名称:	FM327Transceive
// ** 函数功能:	模拟-4卡
// ** 输入参数:    
// ** 输出参数:    无
// ** 返回值:      无
// *********************************************************************************************************/

 /*********************************************************************************************************
T-> 9370880286000C    
T-> 95700726A630B7
T-> E080
T-> 0200A4040007D276000085010100        选type4标签
 <- 029000
T-> 0300A4000C02E103                    选cc文件
 <- 039000
T-> 0200B000000F                        读cc
 <- 02000F20003B00340406E104003200009000
T-> 0300A4000C02E104                    选NDEF文件 
 <- 039000
T-> 0200B0000002                        读NDEF文件多长 
 <- 02000F9000
T-> 0200B000020F                        读NDEF文件  02是偏移量，代表从NDEF数据文件的第2个字节才是真的NDEF
T-> C2
*********************************************************************************************************/

uint8_t  i,xlen, xbuf[256];

uint8_t fm327_fifo[600];
uint8_t status_ok[3] = { 0x02, 0x90, 0x00 };
uint8_t status_word[3] = { 0x02, 0x6A, 0x82 };
uint8_t status_word2[3] = { 0x02, 0x6A, 0x00 };
uint8_t capability_container[15] = { 0x00, 0x0F,	//CCLEN
																		0x20,					//Mapping Version
																		0x00, 0xFF,   //MLe    
																		0x00, 0xFF,   //MLc     
																		0x04,					//NDEF消息格式 05的话就是私有    
																		0x06,					//NDEF消息长度     
																		0xE1, 0x04,   //NDEF FILE ID    
																		0x03, 0x84,   //NDEF最大长度     
																		0x00,         //Read Access    
																		0x00};        //Write Access     

uint8_t ndef_file[456] = { 
		0x00,0x0F,                    
		0xD1,0x01,0x0B,0x55,	//NDEF头字节 类型长度 负载长度  类型55  URI
		0x01,0x68,0x75,0x61,	//0x01 "http://www."    6875617765692E636F6D = huawei.com
		0x77,0x65,0x69,0x2E,
		0x63,0x6F,0x6D
		};



#define CLA  1
#define INS  2
#define P1   3    
#define P2   4
#define LC   5
#define DATA 6
		
const uint8_t ndef_capability_container[2] = { 0xE1, 0x03 };
const uint8_t ndef_id[2] = { 0xE1, 0x04 };

typedef enum { NONE, CC_FILE, NDEF_FILE } T4T_FILE;
T4T_FILE current_file;

void FM11T4T(void)
{
  uint8_t NAK_CRC_ERR = 0x05;
  if(crc_err) 
	{
		FM11_WriteFIFO(&NAK_CRC_ERR, 1);
		FM11_WriteReg(RF_TXEN_REG, 0x55);
		crc_err = 0;
  }	
  else 
	{
		//PCB字段 02、03交替
		status_ok[0] = fm327_fifo[0];
		status_word[0] = fm327_fifo[0];
		status_word2[0] = fm327_fifo[0];
		
    if(fm327_fifo[INS] == 0xA4)  // SELECT
		{
			if(fm327_fifo[P1] == 0x00)
			{
				if ((fm327_fifo[LC] == sizeof(ndef_capability_container)) && (0 == memcmp(ndef_capability_container, fm327_fifo + DATA, fm327_fifo[LC]))) //比较E103
				{
					FM11_WriteFIFO(status_ok, 3);
					FM11_WriteReg(RF_TXEN_REG, 0x55);
					current_file = CC_FILE;	//标识CC
				} 
				else if ((fm327_fifo[LC] == sizeof(ndef_id)) && (0 == memcmp(ndef_id, fm327_fifo + DATA, fm327_fifo[LC]))) {	//比较E104
					FM11_WriteFIFO(status_ok, 3);
	        FM11_WriteReg(RF_TXEN_REG, 0x55);
					current_file = NDEF_FILE;	//标识NDEF
				} 
				else {
					FM11_WriteFIFO(status_word2, 3);
	        FM11_WriteReg(RF_TXEN_REG, 0x55);
					current_file = NONE;
				}
			}
			else if(fm327_fifo[P1] == 0x04) {
        FM11_WriteFIFO(status_ok, 3);
	      FM11_WriteReg(RF_TXEN_REG, 0x55);
			} 
			else {
				FM11_WriteFIFO(status_ok, 3);
		    FM11_WriteReg(RF_TXEN_REG, 0x55);
			}
		}
		else if(fm327_fifo[INS] == 0xB0) {	//READ
			if(current_file == CC_FILE)
			{
				FM11_WriteFIFO(status_ok, 1);
				FM11_WriteFIFO(capability_container + (fm327_fifo[P1] << 8) + fm327_fifo[P2], fm327_fifo[LC]);
			  FM11_WriteFIFO(&status_ok[1], 2);
	      FM11_WriteReg(RF_TXEN_REG, 0x55);
			} 
			else if(current_file == NDEF_FILE) {
				memcpy(xbuf, &status_ok[0], 1);
				memcpy(xbuf+1, ndef_file + (fm327_fifo[P1] << 8) + fm327_fifo[P2], fm327_fifo[LC]);
				memcpy(xbuf+fm327_fifo[LC]+1, status_ok+1, 2);
	      xlen=fm327_fifo[LC]+3;
				FM11_RFDataTx(xlen, xbuf);
			} 
			else { 
				FM11_WriteFIFO(status_word, 3);
	      FM11_WriteReg(RF_TXEN_REG, 0x55);
			}
    } 
		else if(fm327_fifo[INS] ==  0xD6) {	//UPDATE				
      memcpy(ndef_file + (fm327_fifo[P1] << 8) + fm327_fifo[P2], fm327_fifo + DATA, fm327_fifo[LC]);
      FM11_WriteFIFO(status_ok, 3);
		  FM11_WriteReg(RF_TXEN_REG, 0x55);
    } 
		else {
				FM11_RFDataTx(rfLen, fm327_fifo);	//回发接收数据
		}
  }
	
}






