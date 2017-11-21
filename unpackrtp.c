#include <stdio.h>  
#include <stdlib.h> 
#include <math.h>
#include "unpackrtp.h"
unsigned  char timestamp_no[4];//timestamp
unsigned  char seq_no[2]; //序号
int  UnpackRTPH264(unsigned char *bufIn,int len,struct packet *avpacket)
{
int outlen=0;
     if  (len  <  RTP_HEADLEN)         return  -1 ;
    
     //rtp头解析
	memcpy(timestamp_no,(unsigned  char * )bufIn+4,4);
	//avpacket->timestamp=(int64_t)((timestamp_no[0]<<24)+((timestamp_no[1]<<16)+(timestamp_no[2]<<8)))+timestamp_no[3];//有错误，出现负数
	avpacket->timestamp=(int64_t)(timestamp_no[0]*pow(2,24)+timestamp_no[1]*pow(2,16)+timestamp_no[2]*pow(2,8)+timestamp_no[3]);
	memcpy(seq_no,(unsigned  char * )bufIn+2,2);
	avpacket->seq=(seq_no[0]<<8)+seq_no[1];
 
    //h264playload解析
    unsigned  char *src  =  (unsigned  char * )bufIn  +  RTP_HEADLEN;
    unsigned  char  head1  =   * src; // 获取第一个字节
    unsigned  char  nal  =  head1  &   0x1f ; // 获取FU indicator的类型域，
    unsigned  char  head2  =   * (src + 1 ); // 获取第二个字节
    unsigned  char  flag  =  head2  &   0xe0 ; // 获取FU header的前三位，判断当前是分包的开始、中间或结束
    unsigned  char  nal_fua  =  (head1  &   0xe0 )  |  (head2  &   0x1f ); // FU_A nal
	
	avpacket->nal=nal;//输出nal
    if  (nal == 0x1c ){
         if  (flag == 0x80 ) // 开始
         {
//printf("s");
		avpacket->outbuffer[0]=0x0;
		avpacket->outbuffer[1]=0x0;
		avpacket->outbuffer[2]=0x0;
		avpacket->outbuffer[3]=0x1;
		avpacket->outbuffer[4]=nal_fua;
		outlen  = len - RTP_HEADLEN -2+5;//-2跳过前2个字节，+5前面前导码和类型码
		memcpy(avpacket->outbuffer+5,src+2,outlen);
		avpacket->outlen=outlen;
		avpacket->flag=flag;//输出flag
//printf("start:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
        }
         else   if (flag == 0x40 ) // 结束
         {
//printf("e");
	outlen  = len - RTP_HEADLEN -2 ;
	memcpy(avpacket->outbuffer,src+2,len-RTP_HEADLEN-2);
	avpacket->outlen=outlen;
	avpacket->flag=flag;//输出flag
//printf("end:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
       }
         else // 中间
         {
//printf("c");
	outlen  = len - RTP_HEADLEN -2 ;
	memcpy(avpacket->outbuffer,src+2,len-RTP_HEADLEN-2);
	avpacket->outlen=outlen;
	avpacket->flag=flag;//输出flag
//printf("center:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
        }
    }
    else  if  (nal == 0x07 ){//sps
	
//printf("sps");
	avpacket->outbuffer[0]=0x0;
	avpacket->outbuffer[1]=0x0;
	avpacket->outbuffer[2]=0x0;
	avpacket->outbuffer[3]=0x1;
    memcpy(avpacket->outbuffer+4,src,len-RTP_HEADLEN);
    outlen=len-RTP_HEADLEN+4;
    avpacket->outlen=outlen;
	}
    else {//当个包，1,7,8
//printf("*****");
	avpacket->outbuffer[0]=0x0;
	avpacket->outbuffer[1]=0x0;
	avpacket->outbuffer[2]=0x0;
	avpacket->outbuffer[3]=0x1;
    memcpy(avpacket->outbuffer+4,src,len-RTP_HEADLEN);
    outlen=len-RTP_HEADLEN+4;
    avpacket->outlen=outlen;
//printf("singe:bufout[3]=%x %x %x %x,src[0]=%x\n",bufout[3],bufout[4],bufout[5],bufout[6],src[0]);
    }
     return  0;
}

/*
 //备份
int  UnpackRTPH264(unsigned char *bufIn,int len,unsigned char *bufout,struct SPS *sps_buffer)
{
int outlen=0;
     if  (len  <  RTP_HEADLEN)
     {
         return  -1 ;
    }
     //rtp头解析
	memcpy(timestamp_no,(unsigned  char * )bufIn+4,4);
	printf("timestamp=%0d\n",((timestamp_no[0]<<24)+((timestamp_no[1]<<16)+(timestamp_no[2]<<8)))+timestamp_no[3]);
	memcpy(seq_no,(unsigned  char * )bufIn+2,2);
	printf("[%d]:",(seq_no[0]<<8)+seq_no[1]);
    
    //h264playload解析
    unsigned  char *src  =  (unsigned  char * )bufIn  +  RTP_HEADLEN;
    unsigned  char  head1  =   * src; // 获取第一个字节
    unsigned  char  nal  =  head1  &   0x1f ; // 获取FU indicator的类型域，
    unsigned  char  head2  =   * (src + 1 ); // 获取第二个字节
    unsigned  char  flag  =  head2  &   0xe0 ; // 获取FU header的前三位，判断当前是分包的开始、中间或结束
    unsigned  char  nal_fua  =  (head1  &   0xe0 )  |  (head2  &   0x1f ); // FU_A nal
	get_bit_context spsbuffer;
    if  (nal == 0x1c ){
         if  (flag == 0x80 ) // 开始
         {
//printf("s");
      bufout[0]=0x0;
      bufout[1]=0x0;
      bufout[2]=0x0;
      bufout[3]=0x1;
      bufout[4]=nal_fua;
      outlen  = len - RTP_HEADLEN -2+5;//-2跳过前2个字节，+5前面前导码和类型码
	   memcpy(bufout+5,src+2,outlen);
//printf("start:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
        }
         else   if (flag == 0x40 ) // 结束
         {
//printf("e");
outlen  = len - RTP_HEADLEN -2 ;
memcpy(bufout,src+2,len-RTP_HEADLEN-2);
//printf("end:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
       }
         else // 中间
         {
//printf("c");
outlen  = len - RTP_HEADLEN -2 ;
memcpy(bufout,src+2,len-RTP_HEADLEN-2);
//printf("center:bufout[end]=%x %x %x %x,src[end]=%x\n",bufout[outlen-4],bufout[outlen-3],bufout[outlen-2],bufout[outlen-1],src[len-RTP_HEADLEN-1]);
        }

    }
    else  if  (nal == 0x07 ){//sps
	
//printf("sps");
	bufout[0]=0x0;
	bufout[1]=0x0;
	bufout[2]=0x0;
	bufout[3]=0x1;
    memcpy(bufout+4,src,len-RTP_HEADLEN);
    outlen=len-RTP_HEADLEN+4;
    
    //下面是解析sps,返回sps_buffer
    spsbuffer.buf = src+1;
	spsbuffer.buf_size =len-RTP_HEADLEN-1;
	if(h264dec_seq_parameter_set(&spsbuffer, sps_buffer)!=0)   printf("dec sps error\n");
	}
    else {//当个包，1,7,8
//printf("*****");
	bufout[0]=0x0;
	bufout[1]=0x0;
	bufout[2]=0x0;
	bufout[3]=0x1;
    memcpy(bufout+4,src,len-RTP_HEADLEN);
    outlen=len-RTP_HEADLEN+4;
//printf("singe:bufout[3]=%x %x %x %x,src[0]=%x\n",bufout[3],bufout[4],bufout[5],bufout[6],src[0]);
    }
     return  outlen;
}
*/
