# MrbWrap

MrbWrap provides C++ template methods in order to wrap classes directly into mruby without dealing with format strings, type conversions and 10 lines of code, just to wrap a simple integer attribute.

This library is designed to provide the backend for the [Shidacea game engine](https://github.com/Hadeweka/Shidacea). It can also be used as a tool to create mruby gems for C++ libraries easily.

# Requirements

In order to use this library, mruby needs also to be included (for example using CMake). Then, MrbWrap.hpp needs to be included as well in order to use the wrappers. Make also sure to compile this code with C++17 and the standard library.

Otherwise, no other dependencies are needed.

# Usage

MrbWrap provides several methods in order to wrap C++ classes and methods into mruby. For example, take the following C++ example class:

```c++
class Enemy {
public:
    Enemy() = default;
    
    Enemy(int type) {

        if(type == 1) {

            hp = 10;
            atk = 3;

        } else if (type == 2) {

            hp = 15;
            atk = 2;

        } else {

            hp = 0;
            atk = 0;

        }

    }

    ~Enemy() = default;

    unsigned int hp = 0;
    unsigned int atk = 0;

    void get_attacked_by(Enemy& other) {

        unsigned int damage = other.atk;

        if(damage >= hp) {

            hp = 0;

        } else {

            hp -= damage;

        }

    }

};
```

Now you want to wrap this class to mruby. The following code does all that for you:

```c++
void setup_ruby_class_enemy(mrb_state* mrb, RClass* ruby_module) {

    //! Create mruby class of Enemy
    MrbWrap::wrap_class_under<Enemy>(mrb, "Enemy", ruby_module);

    //! Wrap constructor with optional first argument (default value 0)
    MrbWrap::wrap_constructor<Enemy, MRBW_OPT<int, 0>>(mrb);

    //! Wrap attribute hp into getter and setter methods
    MrbWrap::wrap_getter<Enemy, &Enemy::hp>(mrb, "hp");
    MrbWrap::wrap_setter<Enemy, &Enemy::hp, unsigned int>(mrb, "hp=");

    //! Wrap attribute atk into getter and setter methods
    MrbWrap::wrap_getter<Enemy, &Enemy::hp>(mrb, "atk");
    MrbWrap::wrap_setter<Enemy, &Enemy::hp, unsigned int>(mrb, "atk=");

    //! Wrap method directly into an mruby method
    MrbWrap::wrap_member_function<Enemy, &Enemy::get_attacked_by, Enemy>(mrb, "get_attacked_by");

}
```

You now only need to call the setup method once, after opening an instance of mruby and defining a suitable module you want to put the class into.

```c++
//! Start interpreter
auto mrb = mrb_open();

//! Declare mruby module Test
auto test_module = mrb_define_module(mrb, "Test");

//! Activate wrapping methods
setup_ruby_class_enemy(mrb, test_module);
```

It is now possible to run the following mruby code inside the interpreter:

```ruby
# Create a bunch of enemies
enemy_0 = Test::Enemy.new
enemy_1 = Test::Enemy.new(1)
enemy_2 = Test::Enemy.new(2)

# Display attributes
puts "enemy_0 has #{enemy_0.hp} HP"
puts "enemy_1 has #{enemy_1.hp} HP"
puts "enemy_2 has #{enemy_2.atk} ATK"

# Poor enemy 1 gets attacked by enemy 2
puts "Attack!"
enemy_1.get_attacked_by(enemy_2)

# Not so bad
puts "enemy_1 has #{enemy_1.hp} HP"

# Enemy 2 boosts a bit
enemy_2.atk *= 3

# Oh, not again
puts "Attack!"
enemy_1.get_attacked_by(enemy_2)

# Ouch, that hurt
puts "enemy_1 has #{enemy_1.hp} HP"
```

# Limitations

Not every arbitrary class can be wrapped using this library. At the moment, the following argument or attribute data types can be safely wrapped:
* Integer types
* const char*
* std::string
* bool

Floating point types are also possible, but if default arguments are required, the following default wrapper needs to be used, for example, to set a float argument with default value 1.5:

```c++
MRBW_RAT_OPT<float, 3, 2>
```

This is a limitation due to the fact that floating point types are not allowed as template arguments.

Most classes and structs are also easily integratable, but the following limitations apply:
* A standard constructor with no arguments must be specified.
* Private or protected methods and attributes can't be wrapped
* Pointers are currently not supported, so the method must be written manually
* Friend methods are currently not supported
* Default arguments for structs and classes are currently not supported
* Overloaded methods might need further specification (see below)

For overloaded methods, the following template can be used instead of the original function reference, if a new enemy class NewEnemy is declared, which can be only attacked by two enemies:

```c++
MrbWrap::specify<NewEnemy, void, Enemy, Enemy>(&NewEnemy::get_attacked_by)
```

It is still always possible to write a method wrapper manually:

```c++
MrbWrap::define_member_function(mrb, MrbWrap::get_class_info_ptr<Enemy>(), "heal", MRUBY_FUNC {

    auto args = MrbWrap::get_converted_args<unsigned int>(mrb);
    auto heal_value = std::get<0>(args);
    
    auto enemy = MrbWrap::convert_from_object<Enemy>(mrb, self);
    
    enemy->hp += heal_value;
    
    return mrb_nil_value();

}, MRB_ARGS_REQ(1));
```

Compared to completely manual definition, some helper methods were used here, too. This still allows for free control over the function, but removes some of the typical work, especially obtaining arguments via format strings and obtaining the internal representation of the object.
