#include <tuple>
#include <cstdio>
#include <type_traits>
#include <algorithm>
#include <array>
#include <string>
#include <memory>
#include <exception>

struct not_def_constr_t
{
	not_def_constr_t(int val) { std::printf("not_def_constr_t %d\n", val); }
};

not_def_constr_t not_def_contr_ret_val(int &a, int &b, int &c)
{
	a *= 2;
	b *= 4;
	c *= 8;
	return not_def_constr_t(a*b*c);
}

void empty_f()
{
}

void change_i(int &a, int &b, int &c)
{
	a +=10;
	b +=20;
	c +=30;
}


/////////////////////////////// магия
struct transaction_status_t
{
	operator bool() const
	{
		return ok;
	}
	operator std::string() const
	{
		return err;
	}
	bool ok;
	std::string err;
};

// функция не работает для массивов си стиля, вроде int array[256], надо использовать std::array<int,256> - тогда работает, либо указатель и размер 
// возвращаемое значение transaction_status_t если функция возращает void или тип у которого нет конструктора по умолчанию, 
// возвращаемое значение пара из transaction_status_t и возращаемого значения функции иначе  
template<class F, class... Ts>
decltype(auto) transaction(F&& func, Ts&&... vals) 
{
	constexpr bool is_vals_copy_constructible = (true && ... && std::is_copy_constructible_v<std::remove_reference_t<Ts>>);
	static_assert(is_vals_copy_constructible, "all values must be copy constructible after remove reference");
	std::tuple<std::remove_reference_t<Ts>...> tuple(vals...);

	using f_ret_val_type = std::invoke_result_t<F,Ts...>;
	constexpr bool f_has_default_contractible_ret_val = (!std::is_same_v<f_ret_val_type, void> && std::is_default_constructible_v<f_ret_val_type>);

	try
	{
		if constexpr (f_has_default_contractible_ret_val)
		{
			return std::pair(transaction_status_t{true}, func(std::forward<Ts>(vals)...));
		}
		else
		{
			func(std::forward<Ts>(vals)...);
			return transaction_status_t{true};
		}
	}
	catch(std::exception& e)
	{
		constexpr bool is_vals_copy_assignable = (true && ... && std::is_copy_assignable_v<std::remove_reference_t<Ts>>);
		static_assert(is_vals_copy_assignable, "all values must be copy assignable after remove reference");
		std::tie(vals...) = tuple;
		if constexpr (f_has_default_contractible_ret_val)
		{
			return std::pair(transaction_status_t{false,e.what()}, f_ret_val_type());
		}
		else
		{
			return transaction_status_t{false,e.what()};
		}
	}
	catch(...)
	{
		if constexpr (f_has_default_contractible_ret_val)
		{
			return std::pair(transaction_status_t{false,"legacy exception occured"}, f_ret_val_type());
		}
		else
		{
			return transaction_status_t{false,"legacy exception occured"};
		}
	}
}
/////////////////////////////// конец магии 

template<class... Ts>
int throw_f(Ts&&... vals)
{
	// fill with junk
	std::tuple< std::remove_reference_t<Ts>... > tuple{};
	std::tie(vals...) = tuple;
	throw 1;
}

void empty_f2(std::unique_ptr<int> uptr)
{
}

int main()
{
	constexpr char c_arr[] = "hello %d %d %d\n";
	std::array <char, std::size(c_arr)> arr; 
	std::copy(std::begin(c_arr), std::end(c_arr), arr.begin());
	auto str = arr.data();
	int a = 1, b = 2, c = 3;
//	auto ok_empty_f2 = transaction(empty_f2, std::unique_ptr<int>());
	auto ok_empty_f = transaction(empty_f);
	auto ok_change_i = transaction(change_i, a, b, c);
	std::printf("chack values after change_i str=%p, a=%d, b=%d, c=%d\n", str , a, b, c);
	auto [ok_printf, ret_val_of_printf ] = transaction(std::printf, str , a, b, c);
	std::printf("chack values after std::printf str=%p, a=%d, b=%d, c=%d\n", str , a, b, c);
	auto [ok_throw_f, ret_val_of_throw_f ] = transaction(throw_f<char*&,int&,int&,int&>, str , a, b, c);
	std::printf("chack values after throw_f<char*&,int&,int&,int&> str=%p, a=%d, b=%d, c=%d\n", str , a, b, c);
	auto ok_not_def_contr_ret_val = transaction(not_def_contr_ret_val , a, b, c);
	std::printf("chack values after not_def_contr_ret_val str=%p, a=%d, b=%d, c=%d\n", str , a, b, c);

	std::printf(
	"ok_empty_f %d, ok_change_i %d, [ok_printf %d, ret_val_of_printf %d], [ok_throw_f %d, ret_val_of_throw_f %d ], ok_not_def_contr_ret_val %d \n",
	 ok_empty_f.ok, ok_change_i.ok,  ok_printf.ok, ret_val_of_printf,      ok_throw_f.ok, ret_val_of_throw_f,      ok_not_def_contr_ret_val.ok
	);

	return 0;
}

