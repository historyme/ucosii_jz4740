typedef unsigned int (*PFUNC_EXEPROG)(int handle,int * hparam,int *lparam);
typedef struct
{
	char type[4];
	char desc[10];
	PFUNC_EXEPROG ExeProg;
}RELATEFILE,*PRELATEFILE;
#define MAX_RELATE_NUM 5
RELATEFILE m_relate[MAX_RELATE_NUM];
static unsigned char curnum = 0;
int AddRelateFile(char *rltype,char *desc,PFUNC_EXEPROG pProg)
{
	if(curnum < MAX_RELATE_NUM)
	{
		strncpy(m_relate[curnum].type,rltype,4);
		strcpy(m_relate[curnum].desc,desc);
		m_relate[curnum].ExeProg = pProg;
		curnum ++;
		return 1;
	}
	return 0;
}
int SeekRelate(char *rltype)
{
	unsigned char i;
	for(i = 0;i < curnum;i++)
		if(strcmp((char *)m_relate[i].type,rltype) == 0)
			return (unsigned int)i;
	return -1;	
}
int ExeRelateProg(int id,int handle,int *hparam,int *lparam)
{
	if(id < curnum)
	{
		return m_relate[id].ExeProg(handle,hparam,lparam);
	}
	return 0;
}
char *GetRelateDesc(int id)
{
	if(id < curnum)
		return m_relate[id].desc;
	return 0;
}
