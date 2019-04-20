#pragma once
namespace cl
{
	template<class TYPE>
	class singleton
	{
	public:
		singleton() {}
		virtual ~singleton() {}
	private:
		static TYPE* _instance;

	public:
		static TYPE* instance()
		{
			if (0 == _instance)
			{
				_instance = new TYPE();
			}
			return _instance;
		}
		static void instance(TYPE *ins)
		{
			destroy();
			_instance = ins;
		}
		static void destroy()
		{
			if (_instance)
			{
				delete _instance;
				_instance = 0;
			}
		}

	private:
		singleton(const singleton&) {}
		singleton& operator=(const singleton&) {}

	};
	template<class TYPE> TYPE* singleton<TYPE>::_instance = 0;

}
