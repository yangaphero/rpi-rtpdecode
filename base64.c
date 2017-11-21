#include <stdio.h>  
#include <string.h>
#include <stdlib.h> 
#include <malloc.h>
#include "base64.h"   
const   char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";   
    /* Base64 编码 */   
char *base64_encode(const  char* data, int data_len)   
    {   
        //int data_len = strlen(data);   
        int prepare = 0;   
        int ret_len;   
        int temp = 0;   
        char *ret = NULL;   
        char *f = NULL;   
        int tmp = 0;   
        unsigned char changed[4];   
        int i = 0;   
        ret_len = data_len / 3;   
        temp = data_len % 3;   
        if (temp > 0)   
        {   
            ret_len += 1;   
        }   
        ret_len = ret_len*4 + 1;   
        ret = ( char *)malloc(ret_len);   
            
        if ( ret == NULL)   
        {   
            printf("No enough memory.\n");   
            exit(0);   
        }   
        memset(ret, 0, ret_len);   
        f = ret;   
        while (tmp < data_len)   
        {   
            temp = 0;   
            prepare = 0;   
            memset(changed, '\0', 4);   
            while (temp < 3)   
            {   
                //printf("tmp = %d\n", tmp);   
                if (tmp >= data_len)   
                {   
                    break;   
                }   
                prepare = ((prepare << 8) | (data[tmp] & 0xFF));   
                tmp++;   
                temp++;   
            }   
            prepare = (prepare<<((3-temp)*8));   
            //printf("before for : temp = %d, prepare = %d\n", temp, prepare);   
            for (i = 0; i < 4 ;i++ )   
            {   
                if (temp < i)   
                {   
                    changed[i] = 0x40;   
                }   
                else   
                {   
                    changed[i] = (prepare>>((3-i)*6)) & 0x3F;   
                }   
                *f = base[changed[i]];   
                //printf("%.2X", changed[i]);   
                f++;   
            }   
        }   
        *f = '\0';   
            
        return ret;   
            
    }   
      
    /* 转换算子 */   
    static char find_pos( char ch)     
    {   
         char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[]   
        return (ptr - base);   
    }   
      
    /* Base64 解码 
    char *base64_decode(const  char *data, int data_len)   
    {   
        int ret_len = (data_len / 4) * 3;   
        int equal_count = 0;   
        char *ret = NULL;   
        char *f = NULL;   
        int tmp = 0;   
        int temp = 0;  
        int prepare = 0;   
        int i = 0;   
        if (*(data + data_len - 1) == '=')   
        {   
			printf("= ");
            equal_count += 1;   
        }   
        if (*(data + data_len - 2) == '=')   
        {   
			printf("= ");
            equal_count += 1;   
        }   
        if (*(data + data_len - 3) == '=')   
        {//seems impossible   
            equal_count += 1;   
        }   
        switch (equal_count)   
        {   
        case 0:   
            ret_len += 4;//3 + 1 [1 for NULL]   
            break;   
        case 1:   
            ret_len += 4;//Ceil((6*3)/8)+1   
            break;   
        case 2:   
            ret_len += 3;//Ceil((6*2)/8)+1   
            break;   
        case 3:   
            ret_len += 2;//Ceil((6*1)/8)+1   
            break;   
        }   
        ret = ( char *)malloc(ret_len);   
        if (ret == NULL)   
        {   
            printf("No enough memory.\n");   
            exit(0);   
        }   
        printf("ret_len=%d\n",ret_len);
        memset(ret, 0, ret_len);   
        f = ret;   
        while (tmp < (data_len - equal_count))   
        {   
            temp = 0;   
            prepare = 0;   
            while (temp < 4)   
            {   
                if (tmp >= (data_len - equal_count))   
                {   
                    break;   
                }   
                prepare = (prepare << 6) | (find_pos(data[tmp]));   
                temp++;   
                tmp++;   
            }   
            prepare = prepare << ((4-temp) * 6);   
            for (i=0; i<3 ;i++ )   
            {   
                if (i == temp)   
                {   
                    break;   
                }   
                *f = ( char)((prepare>>((2-i)*8)) & 0xFF);   
                f++;   
            }   
        }   
        *f = '\0';   //这里使用strlen()获取长度，但是在十六进制中有问题，因为中间可能出现0x00
        return ret;   
    }  
*/   
//这个函数有问题，主要是长度问题，不过暂不影响使用
    int base64_decode(const  char *data, int data_len,char *out_data)   
    {   
        int ret_len = (data_len / 4) * 3;   
        int equal_count = 0;   
        char *ret = NULL;   
        char *f = NULL;   
        int tmp = 0;   
        int temp = 0;  
        int prepare = 0;   
        int i = 0;   
        if (*(data + data_len - 1) == '=')   
        {   
            equal_count += 1;   
        }   
        if (*(data + data_len - 2) == '=')   
        {   
            equal_count += 1;   
        }   
        if (*(data + data_len - 3) == '=')   
        {//seems impossible   
            equal_count += 1;   
        }   
        switch (equal_count)   
        {   
        case 0:   
            ret_len += 1;//3 + 1 [1 for NULL]   //长度设置不正确
            break;   
        case 1:   
            ret_len +=3;//Ceil((6*3)/8)+1   
            break;   
        case 2:   
            ret_len += 2;//Ceil((6*2)/8)+1   
            break;   
        case 3:   
            ret_len += 1;//Ceil((6*1)/8)+1   
            break;   
        }   
        ret = ( char *)malloc(ret_len);   
        if (ret == NULL)   
        {   
            printf("No enough memory.\n");   
            exit(0);   
        }   
        memset(out_data, 0, ret_len);   
        f = out_data;   
        while (tmp < (data_len - equal_count))   
        {   
            temp = 0;   
            prepare = 0;   
            while (temp < 4)   
            {   
                if (tmp >= (data_len - equal_count))   
                {   
                    break;   
                }   
                prepare = (prepare << 6) | (find_pos(data[tmp]));   
                temp++;   
                tmp++;   
            }   
            prepare = prepare << ((4-temp) * 6);   
            for (i=0; i<3 ;i++ )   
            {   
                if (i == temp)   
                {   
                    break;   
                }   
                *f = ( char)((prepare>>((2-i)*8)) & 0xFF);   
                f++;   
            }   
        }   
        return ret_len;   
    }  
