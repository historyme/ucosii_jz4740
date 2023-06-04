#include <bsp.h>
#include <jz4740.h>
#include <ucos_ii.h>
#include <key.h>

#define KEY_TASK_STK_SIZE	1024
#define KEY_TASK_PRIO	1

static OS_EVENT *key_sem;
static OS_STK KeyTaskStack[KEY_TASK_STK_SIZE];

PFN_KEYHANDLE UpKey = NULL;
PFN_KEYHANDLE DownKey = NULL;



#define KEY_DETECT_REPEAT_TIME 500
#define KEY_REPEAT_TIME 100
#define KEY_DELAY_TIME 10
#define UpKeyHandle(key) {if(UpKey) UpKey(key);}
#define DownKeyHandle(key) {if(DownKey) DownKey(key);}
static void key_interrupt_handler(u32 arg)
{
   
	for(arg = 0;arg < KEY_NUM;arg++)
		__gpio_mask_irq(KEY_PIN + arg);
	OSSemPost(key_sem);

}
static void KeyTaskEntry(void *arg)
{
    u8 err;
	u16 i,run,count = 0;
	u32 key,oldkey;
	u32 upkey = 0;
    u32 keyrepeat = KEY_DETECT_REPEAT_TIME / KEY_DELAY_TIME;
	
	printf("Key Install \r\n");
	
	while(1)
	{
		OSSemPend(key_sem, 0, &err);
		oldkey = (~REG_GPIO_PXPIN(KEY_PIN / 32)) & KEY_MASK;
        run = 1;
		count = 0;
		keyrepeat = KEY_DETECT_REPEAT_TIME / KEY_DELAY_TIME;
		while(run)
        {
			
			OSTimeDly(KEY_DELAY_TIME);
			key = (~REG_GPIO_PXPIN(KEY_PIN / 32)) & KEY_MASK;
			//printf("reg = 0x%8x key = 0x%x oldkey = 0x%x\r\n",REG_GPIO_PXPIN(KEY_PIN / 32),key,oldkey);
			
			if(key ^ oldkey)
			{
				
				oldkey = key;
				continue;
			}
			
			else
			{
				if(key)
				{
					
                    if(key & (~upkey))
					{
						
						DownKeyHandle(key & (~upkey));
					}
					
					else
					{
						if((key ^ upkey) & upkey)
							UpKeyHandle((key ^ upkey) & upkey);
					}
					if(key == upkey)
					{
						count++;
					
						if(count > keyrepeat)
						{
							count = 0;
						    upkey = 0;
							keyrepeat = KEY_REPEAT_TIME / KEY_DELAY_TIME;
						}
					}else
					{
						count = 0;
						//UpKeyHandle(key);
					    upkey = key;
					}
					
				}else
				{
					if(upkey)
						UpKeyHandle(upkey);
                	run = 0;
					upkey =0;
					
				}
				
				
			}
			
		}
		
		for(i = 0; i < KEY_NUM; i++)
		{

			__gpio_ack_irq(KEY_PIN + i);
			__gpio_unmask_irq(KEY_PIN + i);
			
		}
		
	}
	
}

void KeyInit()
{
    int i;
    key_sem = OSSemCreate(0);
	for(i = 0;i < KEY_NUM;i++)
		request_irq(IRQ_GPIO_0 + KEY_PIN + i, key_interrupt_handler, 0);
	
   	OSTaskCreate(KeyTaskEntry, (void *)0,
		     (void *)&KeyTaskStack[KEY_TASK_STK_SIZE - 1],
		     KEY_TASK_PRIO);
    for(i = 0;i < KEY_NUM;i++)
		__gpio_as_irq_fall_edge(KEY_PIN + i);
	
}


