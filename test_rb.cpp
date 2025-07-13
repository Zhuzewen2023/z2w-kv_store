// #include "stdafx.h"
#include "RB_Tree.h"
#include "RB_Tree.cpp"
#include "kv_log.h"

int main(int argc, char* argv[])
{
	RB_Tree<double>* m_RB_Tree = new RB_Tree<double>(1.0);
	for (int i = 2; i < 10;i++)
	{
		double a = i;
		m_RB_Tree->Insert_Node(a);
		
	}
	for (int i = 9;i >2;i--)
	{
		double a = i;
		if (m_RB_Tree->Delete_Node(a) < 0) {
            KV_LOG("delete node %f failed\n", a);
        }
	}	
	return 0;
	

}

