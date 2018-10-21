/**
 * The MIT License (MIT)
 *
 * Copyright © 2017-2018 Ruben Van Boxem
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include "test.h++"

#include <core/event_loop.h++>

#include <thread>
#include <iostream>

namespace
{
  using skui::test::check;

  struct fixture
  {
    int count = 0;
    void f() { ++count; }

    void test_execute_stop()
    {
      skui::core::event_loop event_loop;
      int exit_code {0};

      std::thread thread([&event_loop, &exit_code] { exit_code = event_loop.execute(); });

      event_loop.push(std::make_unique<skui::core::command>([this] { f(); }));
      event_loop.push(std::make_unique<skui::core::command>([this] { f(); }));
      event_loop.push(std::make_unique<skui::core::command>([this] { f(); }));
      event_loop.stop(1);

      thread.join();

      check(count == 3, "3 commands executed.");
      check(exit_code == 1, "proper exit code returned");
    }

    void test_execute_interrupt()
    {
      skui::core::event_loop event_loop;
      int exit_code {0};

      std::thread thread([&event_loop, &exit_code] { exit_code = event_loop.execute(); });

      std::mutex mutex;
      std::unique_lock lock{mutex};
      std::condition_variable condition;
      bool ready = false;
      event_loop.push(std::make_unique<skui::core::command>([&condition, &lock, &ready]
                                                            { condition.wait(lock, [&ready] { return ready; }); }));
      event_loop.push(std::make_unique<skui::core::command>([this] { f(); }));
      event_loop.interrupt(1);
      ready = true;
      condition.notify_one();

      thread.join();

      check(count == 0, "Interrupt prevents further command execution.");
      check(exit_code == 1, "Interrupt returns proper exit code.");
    }

    void test_construct_execute_stop()
    {
      skui::core::command_queue::commands_type commands;
      commands.push_back(std::make_unique<skui::core::command>([this] { f(); }));
      skui::core::event_loop event_loop{std::move(commands)};
      int exit_code {0};

      std::thread thread([&event_loop, &exit_code] { exit_code = event_loop.execute(); });
      event_loop.stop(1);

      thread.join();

      check(count == 1, "1 commands executed.");
      check(exit_code == 1, "proper exit code returned");
    }

    void test_filter()
    {
      std::size_t number_of_commands = 0;
      auto filter = [&number_of_commands](skui::core::command_queue::commands_type& commands) { number_of_commands = commands.size(); };
      skui::core::command_queue::commands_type commands;
      commands.push_back(std::make_unique<skui::core::command>([this] { f(); }));

      skui::core::event_loop event_loop{filter, std::move(commands)};

      event_loop.stop(); // puts event in queue to stop
      event_loop.execute();

      check(number_of_commands == 2, "filter called on commands in event_loop");
    }
  };
}

int main()
{
  fixture{}.test_execute_stop();
  fixture{}.test_execute_interrupt();
  fixture{}.test_construct_execute_stop();
  fixture{}.test_filter();

  return skui::test::exit_code;
}
