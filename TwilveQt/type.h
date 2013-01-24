#pragma once
//! タイプリスト定義

template <int N, class... TS>
struct TypeAt;
template <int N, class T, class... TS>
struct TypeAt<N, T, TS...> {
	typedef typename TypeAt<N-1, TS...>::type	type;
};

template <class T, class... TS>
struct TypeAt<0,T,TS...> {
	typedef T type;
};

template <class T, int N, class... TS>
struct TypeFind;
template <class T, int N, class T0, class... TS>
struct TypeFind<T,N,T0,TS...> {
	enum { result=TypeFind<T,N+1,TS...>::result };
};
template <class T, int N, class... TS>
struct TypeFind<T,N,T,TS...> {
	enum { result=N };
};
template <class T, int N>
struct TypeFind<T,N> {
	enum { result=-1 };
};

template <class CT, class T>
struct GetFlag;
template <template <class...> class LT, class... LArgs, template <class...> class RT, class RF, class... RArgs>
struct GetFlag<LT<LArgs...>, RT<RF,RArgs...>> {
	enum { result=(1<<TypeFind<RF,0,LArgs...>::result) | GetFlag<LT<LArgs...>, RT<RArgs...>>::result };
};
template <template <class...> class LT, class... LArgs, template <class...> class RT, class... RArgs>
struct GetFlag<LT<LArgs...>, RT<RArgs...>> {
	enum { result=0 };
};

template <class... TS>
struct CType {
	template <class TL>
	struct Flag {
		enum { result=0 };
	};
	enum { size=0 };
};
template <class F, class... TS>
struct CType<F,TS...> {
	typedef F First;
	typedef CType<TS...> Other;

	template <int N>
	struct At {
		static_assert(N<sizeof...(TS)+1,"At: out of index");
		typedef typename TypeAt<N,F,TS...>::type type;
	};
	template <class T>
	struct _Find {
		enum { result= TypeFind<T,0,F,TS...>::result };
	};
	template <class T>
	struct Find : _Find<T> {
		static_assert(_Find<T>::result>=0, "Find: not found");
	};
	template <class T>
	struct Has {
		enum { result= (_Find<T>::result>=0) ? 1:0 };
	};
	template <class TL>
	struct Flag {
		enum {result=GetFlag<CType<F,TS...>, TL>::result};
	};

	enum { size= sizeof...(TS)+1 };
};

template <int N, template <class...> class TT>
struct DuplTypes {
	template <template <class...> class T, class... Args>
	static T<int,Args...> Dupl(T<Args...>) { return T<int,Args...>(); }

	typedef DuplTypes<N-1, TT> DPL;
	static decltype(Dupl(DPL::DoIt())) DoIt() {
		return Dupl(DPL::DoIt());
	}
};
template <template <class...> class TT>
struct DuplTypes<0, TT> {
	static TT<> DoIt() {
		return TT<>();
	}
};
