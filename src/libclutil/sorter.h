#pragma once
#include <vector>

namespace cl
{
	/*
	����:
	������arr[]��������,ascending=true ��ʾ����.false ����.
	����"ð��(bubble)�����㷨".
	��ִ��һ��ѭ��,�����ֵ�Ƶ�����.��¼��󽻻�����λ��.�´���ѭ�����ϴν���λ��.
	*/
	template<typename T>
	void sort_bubble(T arr[], int n, bool ascending = true)
	{
		int i = 0, j, k = n - 1;
		T tmp;
		for (j = k; j>0; j = k)
		{
			k = 0;
			for (i = 0; i<j; ++i)
			{
				if ((ascending && arr[i + 1]<arr[i]) || ((!ascending) && arr[i]<arr[i + 1]))
				{
					tmp = arr[i];
					arr[i] = arr[i + 1];
					arr[i + 1] = tmp;
					k = i;
				}
			}
		}
	}
	template<typename T>
	void sort_bubble(std::vector<T>& arr, bool ascending = true)
	{
		int i = 0, j, k = (int)arr.size() - 1;
		T tmp;
		for (j = k; j>0; j = k)
		{
			k = 0;
			for (i = 0; i<j; ++i)
			{
				if ((ascending && arr[i + 1]<arr[i]) || ((!ascending) && arr[i]<arr[i + 1]))
				{
					tmp = arr[i];
					arr[i] = arr[i + 1];
					arr[i + 1] = tmp;
					k = i;
				}
			}
		}
	}

}
