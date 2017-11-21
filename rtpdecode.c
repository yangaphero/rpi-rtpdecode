#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <pthread.h> 
#include <time.h> 
#include "bcm_host.h"
#include "ilclient.h"
#include "queue.h"
#include "unpackrtp.h"
#include "base64.h"

#include "common.h"

CircleQueue bufferqueue;
pthread_mutex_t mut;

struct rtp_param rtpparam={"127.0.0.1",0,8888,1};
struct dec_param decparam={255,{0,0,1280,720},1,0};
struct SPS sps_buffer;
char *base_sps, *base_pps;
int debug = 0;
int run_stauts=0;

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP
  
int Clock_SetSpeed(COMPONENT_T *clock,int speed){
		//调节速度
		//speed=1000;正常速度
		OMX_TIME_CONFIG_SCALETYPE scaleType;
		OMX_INIT_STRUCTURE(scaleType);
		scaleType.xScale = (speed << 16) / 1000;//speed=0为暂停
		if(OMX_SetConfig(ILC_GET_HANDLE(clock),OMX_IndexConfigTimeScale, &scaleType)!= OMX_ErrorNone)
			printf("[clock]OMX_IndexConfigTimeScale error\n");
		return 0;
}

void dec_thread(struct dec_param *param)
{

   COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
   COMPONENT_T *list[5];
   TUNNEL_T tunnel[4];
   ILCLIENT_T *client;
   int status = 0;
   unsigned int data_len = 0;
//BOOL m_settings_changed = FALSE;
   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((client = ilclient_init()) == NULL)      exit(1) ;

   if(OMX_Init() != OMX_ErrorNone)
   {
      ilclient_destroy(client);
      exit(1) ;
   }

   // create video_decode
   if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != OMX_ErrorNone)
      status = -14;
   list[0] = video_decode;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != OMX_ErrorNone)
      status = -14;
   list[1] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != OMX_ErrorNone)
      status = -14;
   list[2] = clock;

   // use automatic clock start time
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      status = -13;
 /*     
  //自定义
	OMX_TIME_CONFIG_TIMESTAMPTYPE sClientTimeStamp;
	OMX_INIT_STRUCTURE(sClientTimeStamp);
	if(OMX_GetConfig(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeCurrentWallTime, &sClientTimeStamp)!=OMX_ErrorNone)
	printf("OMX_GetConfig OMX_IndexConfigTimeClientStartTime error\n!");
	sClientTimeStamp.nPortIndex=80;
	if( OMX_SetConfig(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClientStartTime, &sClientTimeStamp)!=OMX_ErrorNone)
	printf("OMX_SetConfig OMX_IndexConfigTimeClientStartTime error\n!");
*/
   // create video_scheduler
   if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != OMX_ErrorNone)
      status = -14;
   list[3] = video_scheduler;

   set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
   set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
      status = -15;
   else
      ilclient_change_component_state(clock, OMX_StateExecuting);

   if(status == 0)
      ilclient_change_component_state(video_decode, OMX_StateIdle);

   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   OMX_INIT_STRUCTURE(format);
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 130;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;
   format.xFramerate = 25 * (1<<16);//帧速率
   if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format)!= OMX_ErrorNone)
	  printf("OMX_SetParameter OMX_IndexParamVideoPortFormat error\n!");


	OMX_PARAM_PORTDEFINITIONTYPE portParam;
	 OMX_INIT_STRUCTURE(portParam);
	portParam.nPortIndex = 130;
	if(OMX_GetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_GetParameter OMX_IndexParamPortDefinition error\n!");

	portParam.nPortIndex = 130;
	//portParam.nBufferSize=204800;//默认81920,不要轻易改动
	portParam.nBufferCountMin=10;
	portParam.nBufferCountActual=100;
	//portParam.format.video.nFrameWidth  = 1024;
	//portParam.format.video.nFrameHeight = 464;
	if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_SetParameter OMX_IndexParamPortDefinition error\n!");
	if(OMX_GetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_GetParameter OMX_IndexParamPortDefinition error\n!");
		//printf("portParam.nBufferSize=%d\n",portParam.nBufferSize);


  //设置显示模式
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex =90;

	/*
	configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_ALPHA | OMX_DISPLAY_SET_TRANSFORM | OMX_DISPLAY_SET_LAYER | OMX_DISPLAY_SET_NUM);
	configDisplay.alpha = 200;
	configDisplay.num = 0;
	configDisplay.layer = 1;
	configDisplay.transform =0;//0正常 1镜像 2旋转180
	
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_TRUE;

	configDisplay.set                 = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_SRC_RECT|OMX_DISPLAY_SET_FULLSCREEN|OMX_DISPLAY_SET_NOASPECT);
	configDisplay.dest_rect.x_offset  =0;
	configDisplay.dest_rect.y_offset  = 0;
	configDisplay.dest_rect.width     = 640;
	configDisplay.dest_rect.height    = 480;

	configDisplay.src_rect.x_offset   =640;
	configDisplay.src_rect.y_offset   =0;
	configDisplay.src_rect.width      = 640;
	configDisplay.src_rect.height     =480;
	*/

	if(OMX_SetConfig(ILC_GET_HANDLE(video_render),OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
		printf("[video_render]OMX_IndexConfigDisplayRegion error\n");

   //有效帧开始
	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
	OMX_INIT_STRUCTURE(concanParam);
	concanParam.bStartWithValidFrame = OMX_TRUE;//OMX_FALSE OMX_TRUE
	if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam)!= OMX_ErrorNone)
		printf("OMX_SetParameter OMX_IndexParamBrcmVideoDecodeErrorConcealment error\n!");

	//request portsettingschanged on aspect ratio change
	OMX_CONFIG_REQUESTCALLBACKTYPE notifications;
	OMX_INIT_STRUCTURE(notifications);
	notifications.nPortIndex = 131;//OutputPort
	notifications.nIndex = OMX_IndexParamBrcmPixelAspectRatio;
	notifications.bEnable = OMX_TRUE;
	if (OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexConfigRequestCallback, &notifications) != OMX_ErrorNone)
		printf("[video_decode]OMX_SetParameter OMX_IndexConfigRequestCallback error\n!");

/*
	OMX_CONFIG_BOOLEANTYPE timeStampMode;
	OMX_INIT_STRUCTURE(timeStampMode);
	timeStampMode.bEnabled = OMX_TRUE;
	if (OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode) != OMX_ErrorNone)
	 printf("[video_decode]OMX_SetParameter OMX_IndexParamBrcmVideoTimestampFifo error\n!");
*/

   if(status == 0 &&   ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
   {
      OMX_BUFFERHEADERTYPE *buf;
      int port_settings_changed = 0;
      int first_packet = 1;
      ilclient_change_component_state(video_decode, OMX_StateExecuting);
      
      //-----------------------------------------------------------------------------------------------------------
	  frame  content2;
	  int alpha_tmp;
	  struct  rect rect_temp;
	  rect_temp=param->videorect;
	  alpha_tmp=param->alpha;
	  float pts,recpts;

      while(param->thread_exit)
      {
		usleep(0);//防止cpu占用100%，不知道原因
		if (IsQueueEmpty(&bufferqueue))  	{ usleep(500);  continue;}
		
		if(param-> m_settings_changed){//设置发生变化，用于切换流
			port_settings_changed = 0;
			param-> m_settings_changed=0;
		}
	
		//pts获取
		OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
		OMX_INIT_STRUCTURE(timeStamp);
		timeStamp.nPortIndex =80;//OMX_IndexConfigTimeCurrentMediaTime
		if(OMX_GetConfig(ILC_GET_HANDLE(clock),OMX_IndexConfigTimeCurrentMediaTime, &timeStamp)== OMX_ErrorNone){
			pts = (double)FromOMXTime(timeStamp.nTimestamp);
			//if (debug)printf("pts:%.2f [%.0f]\n",  (double)pts* 1e-6, (double)pts);
		}
	
		if(alpha_tmp!=param->alpha){//设置视频透明度
		printf("set alpha:[%d]\n",param->alpha);
			//m_settings_changed = TRUE;
			alpha_tmp=param->alpha;
			OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
			OMX_INIT_STRUCTURE(configDisplay);
			configDisplay.nPortIndex =90;
			configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_ALPHA );
			configDisplay.alpha = param->alpha;
			if(OMX_SetConfig(ILC_GET_HANDLE(video_render),OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
				printf("[video_render]OMX_IndexConfigDisplayRegion error\n");
		}
	  		if(rect_temp.x1!=param->videorect.x1){//设置视频位置，目前只是判断x1的值是否变化
			printf("set rect:[%d,%d,%d,%d]\n",param->videorect.x1,param->videorect.y1,param->videorect.x2,param->videorect.y2);
			//m_settings_changed = TRUE;
			rect_temp=param->videorect;
			OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
			OMX_INIT_STRUCTURE(configDisplay);
			configDisplay.nPortIndex =90;
			configDisplay.fullscreen = OMX_FALSE;
			configDisplay.noaspect   = OMX_TRUE;
			configDisplay.set                 = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_SRC_RECT|OMX_DISPLAY_SET_FULLSCREEN|OMX_DISPLAY_SET_NOASPECT);
			configDisplay.dest_rect.x_offset  =param->videorect.x1;
			configDisplay.dest_rect.y_offset  = param->videorect.y1;
			configDisplay.dest_rect.width     =  param->videorect.x2;
			configDisplay.dest_rect.height    =  param->videorect.y2;
			if(OMX_SetConfig(ILC_GET_HANDLE(video_render),OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
				printf("[video_render]OMX_IndexConfigDisplayRegion error\n");
		}
	
		if ((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
		{

			pthread_mutex_lock(&mut);
			content2=DeQueue(&bufferqueue);//从队列中读取数据
			memcpy(buf->pBuffer,content2.data,content2.len);
			printf("free:  %p  \n",content2.data);
			if (content2.data) free(content2.data);//释放内存
			content2.data=NULL;
			pthread_mutex_unlock(&mut);
			data_len +=content2.len;//为什么是+=
			buf->nFilledLen = data_len;
			//buf->nTimeStamp = ToOMXTime(content2.timestamp);//设置时间戳
			if (debug)printf("[dec %d]:timestamp=%.0f,pts=%.0f,len=%d,queue=%d\n",bufferqueue.count,(double)content2.timestamp,(double)pts,content2.len,bufferqueue.count);
			//if (debug)printf("[pts]:dec[%.2f]-rec[%.2f]=[%.2f]\n",  ((double)pts-(double)content2.start_timestamp)* 1e-6,	((double)content2.timestamp-(double)content2.start_timestamp)/90000, ((double)content2.timestamp-(double)pts)* 1e-6);
			recpts=((double)content2.timestamp-(double)content2.start_timestamp)/90000;
			//recpts=(double)content2.timestamp/90000;
			if (run_stauts)printf("[pts]:rec[%.2f]-dec[%.2f]=[%.2f]\n",  recpts,(double)pts* 1e-6, recpts-(double)pts* 1e-6) ;
			//printf("[dec]data_len=%d,queue_count=%d\n",data_len,bufferqueue.count);
		
			//调节速度
			float video_fifo=((double)content2.timestamp-(double)content2.start_timestamp)/90000-(double)pts* 1e-6;
			if(video_fifo<0.49){
				Clock_SetSpeed(clock,995);
			}else if(video_fifo>0.51){
				Clock_SetSpeed(clock,1010);
			}
			
		}
	
	
//----------------------------------port_settings_changed---------------------------------------------
         if(port_settings_changed == 0 &&
            ((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
             (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
         {
			printf("-------------***-port_settings_changed--***---------------\n");
			content2.start_pts=pts;
            port_settings_changed = 1;

            if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
            {
               status = -7;
               break;
            }

            ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

            // now setup tunnel to video_render
            if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
            {
               status = -12;
               break;
            }

            ilclient_change_component_state(video_render, OMX_StateExecuting);
         }
       
         if(!data_len)
            break;

        //buf->nFilledLen = data_len;
         data_len = 0;

         buf->nOffset = 0;
         if(first_packet)
         {
/*
	 //设置开始时间戳
	OMX_TIME_CONFIG_TIMESTAMPTYPE sClientTimeStamp;
	OMX_INIT_STRUCTURE(sClientTimeStamp);
	if(OMX_GetConfig(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeCurrentWallTime, &sClientTimeStamp)!=OMX_ErrorNone)
	printf("OMX_GetConfig OMX_IndexConfigTimeClientStartTime error\n!");
	sClientTimeStamp.nPortIndex=80;
	sClientTimeStamp.nTimestamp=ToOMXTime(content2.start_timestamp);//设置开始时间戳
	if( OMX_SetConfig(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClientStartTime, &sClientTimeStamp)!=OMX_ErrorNone)
	printf("OMX_SetConfig OMX_IndexConfigTimeClientStartTime error\n!");
*/
//content2.start_timestamp=content2.timestamp;
//if (debug)printf("start_timestamp=%.0f\n",content2.start_timestamp);
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            first_packet = 0;
         }
         else
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
         if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         {
            status = -6;
            break;
         }
       
      }//while-end

      buf->nFilledLen = 0;
      buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         status = -20;
      // wait for EOS from render
      ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,ILCLIENT_BUFFER_FLAG_EOS, 10000);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(tunnel, 0);
   
   }


   ilclient_disable_tunnel(tunnel);
   ilclient_disable_tunnel(tunnel+1);
   ilclient_disable_tunnel(tunnel+2);
   ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
   ilclient_teardown_tunnels(tunnel);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);

   	printf("dec pthread exit\n");
	pthread_exit(0);
   exit(0) ;
}


void rtp_thread(struct rtp_param *param)
{
	struct sockaddr_in addr;
	int sockfd;	
	int addr_len = sizeof(struct sockaddr_in);		
	unsigned char buffer[2048];
	int recv_len;
	int width,height,framerate;// 视频信息
	int temp_width,temp_height;//视频信息暂存，用于判断是否变化
	struct packet   avpacket;
	frame  avframe;
	unsigned char *tmp_data;//组包临时指针
	int tmp_len;//组包临时长度累加
	// 建立socket，注意必须是SOCK_DGRAM
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket error\n");
	}

    //端口复用
    int flag=1;
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)  {  
        fprintf(stderr, "[%s@%s,%d]:socket setsockopt error\n",__func__, __FILE__, __LINE__);
    }  

    //绑定本地端口
    struct sockaddr_in local;  
    local.sin_family=AF_INET;  
    local.sin_port=htons(param->local_port);            ///监听端口  
    local.sin_addr.s_addr=INADDR_ANY;       ///本机  
    if(bind(sockfd,(struct sockaddr*)&local,sizeof(local))==-1) {
		fprintf(stderr,"[%s@%s,%d]:udp port bind error\n",__func__, __FILE__, __LINE__);
    }
/*
	// 填写sockaddr_in
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(param->dest_port);
	addr.sin_addr.s_addr = inet_addr(param->dest_ip);
	connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	//printf("start recv rtp %s:%d(%d)\n",param->dest_ip,param->dest_port,param->local_port);
*/	
	printf("start recv rtp at localport:%d\n",param->local_port);


//--------解码输入参数的sps pps字符串--------
//char *base_sps="J01AKakYDwBE/LgDUBAQG2wrXvfAQA==";
//char *base_pps="KN4JyA==";

if(base_sps && base_pps){
	unsigned char startcode[4]={0x00,0x00,0x00,0x01};
	
	char sps[200] ;
	int spsnum= base64_decode(base_sps, strlen(base_sps),sps);  
	unsigned char *spsoutbuffer= (unsigned char *)malloc(spsnum+4);//动态分配内存
	memcpy(spsoutbuffer,startcode,4);
	memcpy(spsoutbuffer+4,sps,spsnum);
	avframe.data=spsoutbuffer;
	avframe.len=spsnum+4;
	EnQueue(&bufferqueue,avframe);

	char pps[50];
	int ppsnum=base64_decode(base_pps, strlen(base_pps),pps);
	unsigned char *ppsoutbuffer= (unsigned char *)malloc(ppsnum+4);//动态分配内存
	memcpy(ppsoutbuffer,startcode,4);
	memcpy(ppsoutbuffer+4,pps,ppsnum);
	avframe.data=ppsoutbuffer;
	avframe.len=ppsnum+4;
	EnQueue(&bufferqueue,avframe);
	
	if (debug){
		int i;
		for (i=0;i<spsnum+4;i++)
			printf("%02x ",spsoutbuffer[i]);
		printf("\n");
		for (i=0;i<ppsnum+4;i++)
			printf("%02x ",ppsoutbuffer[i]);
		printf("\n");
	}

		//解析sps
	get_bit_context buf_1;
	SPS sps_1;
	buf_1.buf = (void *)sps+1;
	buf_1.buf_size = spsnum-1;
	h264dec_seq_parameter_set(&buf_1, &sps_1);
	width=h264_get_width(&sps_1);
	height=h264_get_height(&sps_1);
	framerate=h264_get_framerate(&sps_1);
	printf("width=%d,height=%d,framerate=%d\n",width,height,framerate);

}
//------------------------------

usleep(500000);
int first_rtp_packet=1;
	while(param->thread_exit)
	{		
		
		bzero(buffer, sizeof(buffer));
		recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, (socklen_t*)&addr_len);

		avpacket.outbuffer=(unsigned char *)malloc(2048);//动态分配内存
		//printf("alloc1:  %p  \n",avpacket.outbuffer);
		memset(avpacket.outbuffer,0,2048);
		if(recv_len>14)	UnpackRTPH264(buffer,recv_len,&avpacket);//收到的是分片，没有重新组包
		if(avpacket.outlen<=0)   continue;//跳过空包，在重组包时可能有问题
		//将网络单个包avpacket组合成avframe视频帧
		if  (avpacket.nal == 0x1f ){//-多个包-帧-----------就在这里重新组包吧，不是太好, 如果想不组包，将0x1c改为其他数0x1f
			switch (avpacket.flag)
			{
				case 0x80: // 开始包
					tmp_data=(unsigned char *)malloc(81920);//动态分配内存，可能出现大于81920的情况
					//tmp_data=(unsigned char *)realloc(avpacket.outbuffer,81920);
					printf("alloc2:  %p  \n",tmp_data);
					memset(tmp_data,0,81920);
					memcpy(tmp_data,avpacket.outbuffer,avpacket.outlen);
					free(avpacket.outbuffer);
					avpacket.outbuffer=NULL;
					tmp_len=avpacket.outlen;
					continue;
				case 0x00: // 中间包
					//if ((tmp_len+avpacket.outlen)>81920) break;
					memcpy(tmp_data+tmp_len,avpacket.outbuffer,avpacket.outlen);
					free(avpacket.outbuffer);
					avpacket.outbuffer=NULL;
					tmp_len+=avpacket.outlen;
					continue;
				case 0x40: // 结束包
					//if ((tmp_len+avpacket.outlen)>81920) break;
					memcpy(tmp_data+tmp_len,avpacket.outbuffer,avpacket.outlen);
					free(avpacket.outbuffer);
					avpacket.outbuffer=NULL;
					tmp_len+=avpacket.outlen;
					avframe.data=tmp_data;
					avframe.len=tmp_len;
					avframe.timestamp=avpacket.timestamp;
				break;
			}//switch
		}else{//单个帧包
				if  (avpacket.nal  == 0x07 ){//sps 解析sps,返回sps_buffer
					get_bit_context spsbuffer;
					spsbuffer.buf = avpacket.outbuffer+5;//跳过0x00--0x01-0xx7
					spsbuffer.buf_size =avpacket.outlen-5;
						if(h264dec_seq_parameter_set(&spsbuffer, &sps_buffer)==0) {
							temp_width=width;
							temp_height=height;
							width=h264_get_width(&sps_buffer);
							height=h264_get_height(&sps_buffer);
							framerate=h264_get_framerate(&sps_buffer);
							if(temp_width!=width||temp_height!=height){//||temp_framerate!=framerate 不判断framerate
							printf("[sps]:width=%d,height=%d,framerate=%d\n",width,height,framerate);
							decparam.m_settings_changed=1;// 通知解码线程改变
						}
					}
				}//sps
				if(first_rtp_packet){
					 avframe.start_timestamp=avpacket.timestamp;
					 first_rtp_packet=0;
				}
				avframe.data=avpacket.outbuffer;
				avframe.len=avpacket.outlen;
				avframe.timestamp=avpacket.timestamp;
		}

		if (debug) printf("[rec %d]:seq=%ld,timestamp=%.0f,len=%d,queue=%d\n",bufferqueue.count,avpacket.seq,(double)avframe.timestamp,avframe.len,bufferqueue.count);
		pthread_mutex_lock(&mut);
		if(IsQueueFull(&bufferqueue)){	printf(RED"full"NONE"\n");		usleep(100000);		}
		else{ EnQueue(&bufferqueue,avframe);		}
		pthread_mutex_unlock(&mut);

	}//while end
		printf("rtp pthread exit\n");
	pthread_exit(0);

}

void display_usage(void)
{
	printf(
	#include "help.h"
	);
}

int main (int argc, char **argv)
{
   	// options
	int opt = 0;
	static const char *optString = "hdl:s:p:S";
	while ((opt = getopt(argc, argv, optString)) != -1) 
	{
		switch (opt)
		{
			case 'h':
				display_usage();
				return 0;
			case 'd':
				debug = 1;
				break;
			case 'l':
				rtpparam.local_port= atoi(optarg);
				printf("local %d",atoi(optarg));
				break;
			case 's':
				base_sps = optarg;
				break;
			case 'p':
				base_pps = optarg;
				break;
			case 'S':
				run_stauts=1;
				break;				
			default:
				printf("Unknown option\n");
				return -1;
		}
   }

   pthread_t rtpthread,decthread;
   bcm_host_init();
   InitQueue(&bufferqueue);
   if(pthread_create(&rtpthread,NULL,(void *)rtp_thread,(struct rtp_param  *)&rtpparam)<0) printf ("Create rtp pthread error!\n");
   //usleep(1000000);
   if(pthread_create(&decthread,NULL,(void *)dec_thread,(struct dec_param  *)&decparam)<0) printf ("Create decode pthread error!\n");
  int quit_flag=1;
  char command;  
  while(quit_flag){
	printf ("please input the comand:\n");  
	scanf ("%c", &command);  
	getchar ();  
	switch (command)  
	{  
	case 'a':  
		decparam.alpha=255;
		break;
	case 'b':  
		decparam.alpha=100;
		break;
	case 'c':  
		decparam.videorect.x1=200;
		decparam.videorect.y1=100;
		decparam.videorect.x2=840;
		decparam.videorect.y2=580;
		break;
	case 'd':  
		decparam.videorect.x1=0;
		decparam.videorect.y1=0;
		decparam.videorect.x2=1280;
		decparam.videorect.y2=720;
		break;
	case 's':  
		
		break;
	case 'q':  
		rtpparam.thread_exit=0;
		decparam.thread_exit=0;
		quit_flag=0;
		break;
	  }
  }
   
   pthread_join(rtpthread,NULL);
   pthread_join(decthread,NULL);
   printf("aphero have a nice day :)\n");
   return 0;
}


