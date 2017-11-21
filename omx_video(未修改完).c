#include <stdio.h>
#include <stdlib.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "common.h"

CircleQueue bufferqueue;
pthread_mutex_t mut;

int video_init(){
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
	
}

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

void video_decode(struct dec_param *param)
{

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
			pthread_mutex_unlock(&mut);
			memcpy(buf->pBuffer,content2.data,content2.len);
			if (content2.data) free(content2.data);//释放内存
			content2.data=NULL;
			data_len +=content2.len;
			buf->nFilledLen = data_len;
			buf->nTimeStamp = ToOMXTime(content2.timestamp);//设置时间戳
			if (debug)printf("[dec]:timestamp=%.0f,pts=%.0f,len=%d,queue=%d\n",(double)content2.timestamp,(double)pts,content2.len,bufferqueue.count);
			//if (debug)printf("[pts]:dec[%.2f]-rec[%.2f]=[%.2f]\n",  ((double)pts-(double)content2.start_timestamp)* 1e-6,	((double)content2.timestamp-(double)content2.start_timestamp)/90000, ((double)content2.timestamp-(double)pts)* 1e-6);
			recpts=((double)content2.timestamp-(double)content2.start_timestamp)/90000;
			if (debug)printf("[pts]:rec[%.2f]-dec[%.2f]=[%.2f]\n",  recpts,(double)pts* 1e-6, recpts-(double)pts* 1e-6) ;
			if (debug) printf("[%d]-",bufferqueue.count);
			//printf("[dec]data_len=%d,queue_count=%d\n",data_len,bufferqueue.count);
		
			//调节速度
			float video_fifo=((double)content2.timestamp-(double)content2.start_timestamp)/90000-(double)pts* 1e-6;
			if(video_fifo<0.3){
				Clock_SetSpeed(clock,990);
			}else if(video_fifo>0.5){
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


   
   }

   exit(0) ;
}

int video_deinit(){
	
	      buf->nFilledLen = 0;
      buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         status = -20;
      // wait for EOS from render
      ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,ILCLIENT_BUFFER_FLAG_EOS, 10000);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(tunnel, 0);
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
}

