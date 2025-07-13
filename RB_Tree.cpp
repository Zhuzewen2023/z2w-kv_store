// #include "stdafx.h"
#include "RB_Tree.h"
template <class T>
RB_Tree<T>::RB_Tree(T Root_Data):Root_Node(NULL)
{	
	if (Root_Node == NULL)
	{
		Root_Node = new RB_Tree_Node<T>(Root_Data);
		Root_Node->color_tag = 0;
	}
}

template <class T>
RB_Tree<T>::~RB_Tree()
{
}
/************************************************************************/
/* �������ܣ��������в���һ���ڵ�                                     */
// ��ڲ��������������
// ����ֵ����
/************************************************************************/
template <class T>
void RB_Tree<T>::Insert_Node(T insert_data)
{
	
	RB_Tree_Node<T>* temp_Node = Root_Node;
	while(temp_Node != NULL)
	{
		if (insert_data > temp_Node->data)
		{
			if (temp_Node->Right_child == NULL)
			{
				temp_Node->Right_child = new RB_Tree_Node<T>(insert_data);
				temp_Node->Right_child->color_tag = 1;
				temp_Node->Right_child->Father_Node = temp_Node;
				if (temp_Node->color_tag == 1)
				{
					Fix_Tree(temp_Node->Right_child);
				}				
				break;
			}
			else
			{
				temp_Node = temp_Node->Right_child;
			}			
		}
		else
		{
			if (temp_Node->Left_child == NULL)
			{
				temp_Node->Left_child  = new RB_Tree_Node<T>(insert_data);
				temp_Node->Left_child->color_tag = 1;
				temp_Node->Left_child->Father_Node = temp_Node;
				if (temp_Node->color_tag == 1)
				{
					Fix_Tree(temp_Node->Left_child);
				}				
				break;
			}
			else
			{
				temp_Node = temp_Node->Left_child;
			}			
		}
	}	
}

/************************************************************************/
/* �������ܣ��Ӻ��������ѰҪɾ�������ݽڵ�                             */
// ��ڲ�����ɾ��������
// ����ֵ��1��ʾɾ���ɹ� -1��ʾɾ��ʧ��
/************************************************************************/
template<class T>
int RB_Tree<T>::Delete_Node(T delete_data)
{	
	RB_Tree_Node<T>* temp_Node = Root_Node;
	while(temp_Node->data != delete_data && temp_Node != NULL)
	{
		if (delete_data > temp_Node->data)
		{
			temp_Node = temp_Node->Right_child;
		}
		else
		{
			temp_Node = temp_Node->Left_child;
		}
	}	
	if (temp_Node)
	//�ҵ����ص�����
	{
		int color_tag = temp_Node->color_tag;
		
		if (temp_Node->Left_child == NULL && temp_Node->Right_child == NULL)
		//��������Ϊ����ֱ��ɾ��
		{
			//			
			delete temp_Node;
		}
		else
		if (temp_Node->Left_child == NULL && temp_Node->Right_child != NULL)
		//������Ϊ��,��������Ϊ��
		{				
			if (temp_Node != Root_Node)
			//��Ϊ���ڵ�
			{
				if (temp_Node->Father_Node->Left_child == temp_Node)
				{
					temp_Node->Father_Node->Left_child = temp_Node->Right_child;
					temp_Node->Right_child->Father_Node = temp_Node->Father_Node;
					
				}
				else
				{
					temp_Node->Father_Node->Right_child = temp_Node->Right_child;
					temp_Node->Right_child->Father_Node = temp_Node->Father_Node;
					
				}
			}
			else
			//���ڵ�
			{
				Root_Node = temp_Node->Right_child;
				temp_Node->Right_child->Father_Node = Root_Node;
				
			}
		}
		else if (temp_Node->Left_child != NULL && temp_Node->Right_child == NULL)
		//��������Ϊ��,������Ϊ��
		{
			if (temp_Node != Root_Node)
				//��Ϊ���ڵ�
			{
				if (temp_Node->Father_Node->Left_child == temp_Node)
				{
					temp_Node->Father_Node->Left_child = temp_Node->Left_child;
					temp_Node->Left_child->Father_Node = temp_Node->Father_Node;
					
				}
				else
				{
					temp_Node->Father_Node->Right_child = temp_Node->Left_child;
					temp_Node->Left_child->Father_Node = temp_Node->Father_Node;
					
				}
			}
			else
				//���ڵ�
			{
				Root_Node = temp_Node->Left_child;
				temp_Node->Left_child->Father_Node = Root_Node;
				//delete temp_Node;
			}
		}
		else if (temp_Node->Left_child != NULL && temp_Node->Right_child != NULL)
		//������������Ϊ��
		{
			RB_Tree_Node<T>* Successor_Node = Find_Successor_Node(temp_Node);
			if (temp_Node == temp_Node->Father_Node->Left_child)
			{
				temp_Node->Father_Node->Left_child = Successor_Node;
				Successor_Node->Right_child = temp_Node->Right_child;
				Successor_Node->Left_child = temp_Node->Left_child;
				Successor_Node->Father_Node = temp_Node->Father_Node;
				Successor_Node->color_tag = temp_Node->color_tag;
				if (Successor_Node->Right_child)
				{
					Successor_Node->Father_Node->Left_child = Successor_Node->Right_child;
					Successor_Node->Right_child->Father_Node = Successor_Node->Father_Node;
				}
				
			}
			else
			{
				temp_Node->Father_Node->Right_child = Successor_Node;
				Successor_Node->Right_child = temp_Node->Right_child;
				Successor_Node->Left_child = temp_Node->Left_child;
				Successor_Node->Father_Node = temp_Node->Father_Node;
				Successor_Node->color_tag = temp_Node->color_tag;
				if (Successor_Node->Right_child)
				{
					Successor_Node->Father_Node->Left_child = Successor_Node->Right_child;
					Successor_Node->Right_child->Father_Node = Successor_Node->Father_Node;
				}
			}			
		}
		
		if (color_tag == 0)
		//���ɾ���Ľڵ�Ϊ��ɫ  ������������
		{
			Fix_Tree_Delete(temp_Node);
		}
	}
	//δ����ɾ��������  ����-1
	else
	{
		return -1;
	}
	return 1;
	
}

/************************************************************************/
/* �������ܣ�����ڵ���������������֤��������                         */
// ��ڲ���������ĵ�ǰ�ڵ�
// ����ֵ����
/************************************************************************/
template <class T>
void RB_Tree<T>::Fix_Tree(RB_Tree_Node<T>* current_Node)
{
	RB_Tree_Node<T>* temp_current_Node = current_Node;	
	RB_Tree_Node<T>* uncle_Node;	
		
	while(true)
	{
		if ( temp_current_Node->Father_Node == NULL)
		{
			break;
		}
		if (temp_current_Node->Father_Node->color_tag != 1)
		{
			break;
		}		 
		RB_Tree_Node<T>* father_Node = temp_current_Node->Father_Node;
		RB_Tree_Node<T>* grandfa_Node = father_Node->Father_Node;
		if (grandfa_Node)
		{
			if (father_Node == grandfa_Node->Left_child)
			{
				uncle_Node = grandfa_Node->Right_child;
				//���������ڵ�ʱ
				if (uncle_Node)
				{
					//���1 ����Ϊ��ɫ  �����׽ڵ������ڵ�����Ϊ��ɫ 
					//�游�ڵ�����Ϊ��ɫ ���游�ڵ�����Ϊ��ǰ�ڵ�
					if (uncle_Node->color_tag == 1)
					{
						uncle_Node->color_tag = 0;
						father_Node->color_tag = 0;						
						grandfa_Node->color_tag = 1;						
						temp_current_Node = grandfa_Node;						
					}
					//���2�������Ǻ�ɫ �ҵ�ǰ�ڵ�Ϊ�Һ��� �����ڵ���Ϊ��ǰ�ڵ� �Ը��ڵ��������
					else if (temp_current_Node == father_Node->Right_child)
					{
						temp_current_Node = temp_current_Node->Father_Node;						
						Left_Rotate(temp_current_Node);					
					}
					//���3�������Ǻ�ɫ �ҵ�ǰ�ڵ�Ϊ���� �����ڵ���Ϊ��ɫ �游�ڵ���Ϊ��ɫ ���游�ڵ�����
					else
					{
						father_Node->color_tag = 0;
						grandfa_Node->color_tag = 1;
						Right_Rotate(grandfa_Node);				
					}					
				}
				//û������ڵ�ʱ
				else
				{
					if (temp_current_Node == father_Node->Right_child)
					{
						temp_current_Node = temp_current_Node->Father_Node;						
						Left_Rotate(temp_current_Node);					
					}					
					else
					{
						father_Node->color_tag = 0;
						grandfa_Node->color_tag = 1;
						Right_Rotate(grandfa_Node);	
					}
				}
			}
			else
			{
				uncle_Node = grandfa_Node->Left_child;
				if (uncle_Node)
				{
					//���1 ����Ϊ��ɫ  �����׽ڵ������ڵ�����Ϊ��ɫ 
					//�游�ڵ�����Ϊ��ɫ ���游�ڵ�����Ϊ��ǰ�ڵ�
					if (uncle_Node->color_tag == 1)
					{
						uncle_Node->color_tag = 0;
						father_Node->color_tag = 0;						
						grandfa_Node->color_tag = 1;						
						temp_current_Node = grandfa_Node;						
					}
					//���2�������Ǻ�ɫ �ҵ�ǰ�ڵ�Ϊ�Һ��� �����ڵ���Ϊ��ǰ�ڵ� �Ը��ڵ��������
					else if (temp_current_Node == father_Node->Left_child)
					{
						temp_current_Node = temp_current_Node->Father_Node;
						Right_Rotate(temp_current_Node);						
					}
					//���3�������Ǻ�ɫ �ҵ�ǰ�ڵ�Ϊ���� �����ڵ���Ϊ��ɫ �游�ڵ���Ϊ��ɫ ���游�ڵ�����
					else
					{
						father_Node->color_tag = 0;
						grandfa_Node->color_tag = 1;
						Left_Rotate(grandfa_Node);	
					}									
				}
				else
				{
					if (temp_current_Node == father_Node->Left_child)
					{
						temp_current_Node = temp_current_Node->Father_Node;
						Right_Rotate(temp_current_Node);						
					}					
					else
					{
						father_Node->color_tag = 0;
						grandfa_Node->color_tag = 1;
						Left_Rotate(grandfa_Node);	
					}	
				}
			}
		}
	}
	Root_Node->color_tag = 0;

}

/************************************************************************/
/* �������ܣ��Ե�ǰ�ڵ������������                                     */
// ��ڲ����������ĵ�ǰ�ڵ�
// ����ֵ����
/************************************************************************/
template <class T>
void RB_Tree<T>::Left_Rotate(RB_Tree_Node<T>* current_Node)
{
	RB_Tree_Node<T>* Right_child = current_Node->Right_child;	
	RB_Tree_Node<T>* father_Node = current_Node->Father_Node;
	current_Node->Right_child = Right_child->Left_child;
	Right_child->Father_Node = father_Node;
	if (father_Node == NULL)
	{
		Root_Node = Right_child;
	}
	else if(current_Node == father_Node->Left_child)
	{
		father_Node->Left_child = Right_child;
	}
	else
	{
		father_Node->Right_child = Right_child;
	}	

	Right_child->Left_child = current_Node;
	current_Node->Father_Node = Right_child;

}

/************************************************************************/
/* �������ܣ��Ե�ǰ�ڵ������������                                     */
// ��ڲ����������ĵ�ǰ�ڵ�
// ����ֵ����
/************************************************************************/
template <class T>
void RB_Tree<T>::Right_Rotate(RB_Tree_Node<T>* current_Node)
{
	RB_Tree_Node<T>* left_Node = current_Node->Left_child;
	RB_Tree_Node<T>* father_Node = current_Node->Father_Node;
	current_Node->Left_child = left_Node->Right_child;
	left_Node->Right_child = current_Node;
	if (father_Node == NULL)
	{
		Root_Node = left_Node;
	}
	else if (current_Node = father_Node->Left_child)
	{
		father_Node->Left_child = left_Node;
	}
	else
	{
		father_Node->Right_child = left_Node;
	}
	current_Node->Father_Node = left_Node;
	left_Node->Father_Node = father_Node;
}


/************************************************************************/
/* �������ܣ���Ѱ��ǰ�ڵ�������̽ڵ�                                 */
// ��ڲ�������ǰ�ڵ�
// ����ֵ����ǰ�ڵ�������̽ڵ�
/************************************************************************/
template<class T>
RB_Tree_Node<T>* RB_Tree<T>::Find_Successor_Node(RB_Tree_Node<T>* current_Node)
{
	RB_Tree_Node<T>* temp_Node = current_Node->Right_child;
	while(temp_Node->Left_child != NULL)
	{
		temp_Node = temp_Node->Left_child;
	}
	return temp_Node;	
}

/************************************************************************/
/* �������ܣ�����ýڵ���ص�������Ϣ                                   */
// ��ڲ�������ǰ�ڵ�
// ����ֵ����
/************************************************************************/
template<class T>
void RB_Tree<T>::erase_Node(RB_Tree_Node<T>* Node_del)
{
	if (Node_del->Father_Node)
	{
		Node_del->Father_Node = NULL;
	}
	Node_del->color_tag = NULL;
	Node_del->Father_Node = NULL;
	Node_del->Left_child = NULL;
	Node_del->Right_child = NULL;
	Node_del->data = NULL;	
	delete Node_del;
}

/************************************************************************/
/* 功能：删除节点后修复红黑树性质                                       */
// 输入：删除节点的替代节点（或删除节点的子节点）
// 返回值：无
/************************************************************************/
template <class T>
void RB_Tree<T>::Fix_Tree_Delete(RB_Tree_Node<T>* x)
{
    while (x != Root_Node && x->color_tag == 0) {
        if (x == x->Father_Node->Left_child) {
            RB_Tree_Node<T>* w = x->Father_Node->Right_child;
            
            // 情况1：兄弟节点是红色
            if (w->color_tag == 1) {
                w->color_tag = 0;
                x->Father_Node->color_tag = 1;
                Left_Rotate(x->Father_Node);
                w = x->Father_Node->Right_child;
            }
            
            // 情况2：兄弟节点是黑色，且兄弟的两个子节点都是黑色
            if (w->Left_child->color_tag == 0 && w->Right_child->color_tag == 0) {
                w->color_tag = 1;
                x = x->Father_Node;
            } else {
                // 情况3：兄弟节点是黑色，兄弟的左孩子是红色，右孩子是黑色
                if (w->Right_child->color_tag == 0) {
                    w->Left_child->color_tag = 0;
                    w->color_tag = 1;
                    Right_Rotate(w);
                    w = x->Father_Node->Right_child;
                }
                
                // 情况4：兄弟节点是黑色，兄弟的右孩子是红色
                w->color_tag = x->Father_Node->color_tag;
                x->Father_Node->color_tag = 0;
                w->Right_child->color_tag = 0;
                Left_Rotate(x->Father_Node);
                x = Root_Node; // 终止循环
            }
        } else {
            // 对称情况（x是右孩子）
            RB_Tree_Node<T>* w = x->Father_Node->Left_child;
            
            if (w->color_tag == 1) {
                w->color_tag = 0;
                x->Father_Node->color_tag = 1;
                Right_Rotate(x->Father_Node);
                w = x->Father_Node->Left_child;
            }
            
            if (w->Right_child->color_tag == 0 && w->Left_child->color_tag == 0) {
                w->color_tag = 1;
                x = x->Father_Node;
            } else {
                if (w->Left_child->color_tag == 0) {
                    w->Right_child->color_tag = 0;
                    w->color_tag = 1;
                    Left_Rotate(w);
                    w = x->Father_Node->Left_child;
                }
                
                w->color_tag = x->Father_Node->color_tag;
                x->Father_Node->color_tag = 0;
                w->Left_child->color_tag = 0;
                Right_Rotate(x->Father_Node);
                x = Root_Node; // 终止循环
            }
        }
    }
    x->color_tag = 0; // 确保根节点为黑色
}