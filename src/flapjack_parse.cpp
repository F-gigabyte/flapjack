#include <flapjack_parse.h>
#include <stdexcept>
#include <unordered_map>
#include <flapjack_commands.h>

extern char** environ;

VarelseParser::VarelseParser() : streams(
        (TerminalStream)
        {
            .stdin_path = "",
            .stdout_path = "",
            .stdout_append = false,
            .stderr_path = "",
            .stderr_append = false,
        }), background(false)
{
    for(std::size_t i = 0; i < registers.size(); i++)
    {
        registers[i] = "";
    }
}

static size_t parse_index(const std::string& index)
{
    if(index.length() == 0)
    {
        throw std::invalid_argument("Can't cast '" + index + "' to std::size_t");
    }
    size_t res = 0;
    for(char c : index)
    {
        std::size_t next_res = res * 10;
        if(next_res < res)
        {
            throw std::out_of_range("Can't cast '" + index + "' to std::size_t");
        }
        if(c >= '0' && c <= '9')
        {
            res += c - '0';
        }
        else
        {
            throw std::invalid_argument("Can't cast '" + index + "' to std::size_t");
        }
    }
    return res;
}

std::vector<std::string> VarelseParser::split_line(const std::string& text)
{
    std::vector<std::string> res;
    std::string word = "";
    bool in_quotes = false;
    char quote = 0;
    bool escape = false;
    for(size_t i = 0; i < text.length(); i++)
    {
       if(in_quotes)
       {
           if(escape)
           {
               switch(text[i])
               {
                   case 'n':
                   {
                       word += '\n';
                       break;
                   }
                   case 'r':
                   {
                       word += '\r';
                       break;
                   }
                   case 't':
                   {
                       word += '\t';
                       break;
                   }
                   case '\\':
                   {
                       word += '\\';
                       break;
                   }
                   case '\"':
                   {
                       word += '\"';
                       break;
                   }
                   case '\'':
                   {
                       word += '\'';
                       break;
                   }
                   default:
                   {
                       word += '\\';
                       word += text[i];
                       break;
                   }
               }
               escape = false;
           }
           else
           {
               if(text[i] == quote)
               {
                   quote = 0;
                   in_quotes = false;
               }
               else if(text[i] == '\\')
               {
                   escape = true;
               }
               else
               {
                   word += text[i];
               }
           }
       }
       else
       {
           switch(text[i])
           {
               case '\'':
               case '\"':
               {
                   quote = text[i];
                   in_quotes = true;
                   break;
               }
               case ' ':
               {
                   if(word.length() > 0)
                   {
                       res.emplace_back(word);
                       word = "";
                   }
                   break;
               }
               case '\t':
               case '\r':
               case '\n':
               {
                   break;
               }
               default:
               {
                   word += text[i];
                   break;
               }
           }
       }
    }
    if(word.length() > 0)
    {
        res.emplace_back(word);
    }
    return res;
}

bool VarelseParser::get_reg_arg(const std::string& index, std::size_t& arg)
{
    try
    {
        arg = parse_index(index);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool VarelseParser::get_command_args(const std::vector<std::string>& line, std::vector<std::string>& args)
{
    std::vector<std::string> res;
    for(size_t i = 0; i < line.size() - 1; i++)
    {
        std::size_t index;
        if(get_reg_arg(line[i], index))
        {
            res.emplace_back(registers[index]);
        }
        else
        {
            return false;
        }
    }
    args = res;
    return true;
}

void VarelseParser::parse(TerminalIO& terminal, std::string& current_dir, const std::vector<std::string>& lines, std::size_t ip)
{
    std::unordered_map<std::string, size_t> labels;
    for(size_t i = 0; i < lines.size(); i++)
    {
        if(lines[i].ends_with('<'))
        {
            std::vector<std::string> line = split_line(lines[i]);
            std::string op = line.back(); 
            if(op.length() == 1)
            {
                if(line.size() == 2)
                {
                    labels.emplace(line[0], i + 1);
                }
                else
                {
                    terminal.print_error("Invalid instruction '%s'\r\n", lines[i].c_str());
                }
            }
        }
    }
    for(;ip < lines.size() && !terminal.should_quit(); ip++)
    {
        std::vector<std::string> line = split_line(lines[ip]);
        if(line.size() > 0)
        {
            std::string op = line.back(); 
            if(op.length() != 1)
            {
                terminal.print_error("Unknown command '%s'\r\n", op.c_str());
            }
            else
            {
                switch(op[0])
                {
                    case ';':
                    {
                        std::size_t arg1;
                        std::size_t arg2;
                        if(line.size() == 3 && get_reg_arg(line[0], arg1) && get_reg_arg(line[1], arg2))
                        {
                            registers[arg1] = registers[arg2];
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case ':':
                    {
                        std::size_t arg1;
                        if(line.size() == 3 && get_reg_arg(line[0], arg1))
                        {
                            registers[arg1] = line[1];
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '<':
                    {
                        break;
                    }
                    case '>':
                    {
                        std::size_t arg1;
                        std::size_t arg2;
                        if(line.size() == 2 && get_reg_arg(line[0], arg1))
                        {
                            std::string loc = registers[arg1];
                            if(labels.find(loc) != labels.end())
                            {
                                ip = labels.at(loc) - 1; // will add 1 at end of loop
                            }
                            else
                            {
                                terminal.print_error("Invalid jump location '%s'\r\n", loc.c_str());
                            }
                        }
                        else if(line.size() == 3 && get_reg_arg(line[0], arg1) && get_reg_arg(line[1], arg2))
                        {
                            std::string loc = registers[arg1];
                            if(labels.find(loc) != labels.end())
                            {
                                if(registers[arg2].length() > 0)
                                {
                                    ip = labels.at(loc) - 1;    
                                }
                            }
                            else
                            {
                                terminal.print_error("Invalid jump location '%s'\r\n", loc.c_str());
                            }
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '#':
                    {
                        if(line.size() < 2)
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        else
                        {
                            std::vector<std::string> cmd_args;
                            if(get_command_args(line, cmd_args))
                            {
                                exec_process(terminal, background, cmd_args, streams);
                            }
                            else
                            {
                                terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                            }
                        }
                        break;
                    }
                    case '@':
                    {
                        std::vector<std::string> args;
                        if(line.size() == 2)
                        {
                            std::size_t arg1;
                            if(line.size() == 2 && get_reg_arg(line[0], arg1))
                            {
                                args.emplace_back(registers[arg1]);
                                cd_cmd(terminal, current_dir, args);
                            }
                            else
                            {
                                terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                            }
                        }
                        else if(line.size() == 1)
                        {
                            cd_cmd(terminal, current_dir, args);
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '_':
                    {
                        std::vector<std::string> cmd_args;
                        if(get_command_args(line, cmd_args))
                        {
                            dir_cmd(terminal, current_dir, cmd_args);
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case ')':
                    {
                        if(line.size() == 1)
                        {
                            terminal.print("\033[2J\033[H");
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '\\':
                    {
                        std::vector<std::string> cmd_args;
                        if(get_command_args(line, cmd_args))
                        {
                            if(cmd_args.size() != 0)
                            {
                                for(std::size_t i = 0; i < cmd_args.size() - 1; i++)
                                {
                                    terminal.print("%s ", cmd_args[i].c_str());
                                }
                                terminal.print(cmd_args.back().c_str());
                            }
                            terminal.print("\r\n");
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '=':
                    {
                        if(line.size() == 1)
                        {
                            exit(0);
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '-':
                    {
                        if(line.size() == 1)
                        {
                            terminal.set_text_colour(stdout, TerminalColour::LIGHT_PURPLE);
                            terminal.print("Background: %s\r\n", background ? "true" : "false");
                            terminal.set_text_colour(stdout, TerminalColour::LIGHT_GREEN);
                            terminal.print("Stdio\r\n");
                            if(streams.stdin_path.length() > 0)
                            {
                                terminal.print("\t[r] stdin:  '%s'\r\n", streams.stdin_path.c_str());
                            }
                            else
                            {
                                terminal.print("\t[r] stdin:  default\r\n");
                            }
                            if(streams.stdout_path.length() > 0)
                            {
                                terminal.print("\t[%c] stdout: '%s'\r\n", streams.stdout_append ? 'a' : 'w', streams.stdout_path.c_str());
                            }
                            else
                            {
                                terminal.print("\t[%c] stdout: default\r\n", streams.stdout_append ? 'a' : 'w');
                            }
                            if(streams.stderr_path.length() > 0)
                            {
                                terminal.print("\t[%c] stderr: '%s'\r\n", streams.stderr_append ? 'a' : 'w', streams.stderr_path.c_str());
                            }
                            else
                            {
                                terminal.print("\t[%c] stderr: default\r\n", streams.stderr_append ? 'a' : 'w');
                            }
                            terminal.set_text_colour(stdout, TerminalColour::LIGHT_BLUE);
                            terminal.print("Registers\r\n");
                            for(std::size_t i = 0; i < registers.size(); i++)
                            {
                                terminal.print("\t[%zu] \'%s\'\r\n", i, registers[i].c_str());
                            }
                            terminal.reset_text_colour(stdout);
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '(':
                    {
                        std::size_t arg1;
                        if(line.size() == 1)
                        {
                            streams.stdin_path = "";
                        }
                        else if(line.size() == 2 && get_reg_arg(line[0], arg1))
                        {
                            streams.stdin_path = registers[arg1];
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case ']':
                    {
                        std::size_t arg1;
                        if(line.size() == 1)
                        {
                            streams.stdout_path = "";
                        }
                        else if(line.size() == 2 && get_reg_arg(line[0], arg1))
                        {
                            streams.stdout_path = registers[arg1];
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '}':
                    {
                        if(line.size() == 1)
                        {
                            streams.stdout_append = !streams.stdout_append;
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '[':
                    {
                        std::size_t arg1;
                        if(line.size() == 1)
                        {
                            streams.stderr_path = "";
                        }
                        else if(line.size() == 2 && get_reg_arg(line[0], arg1))
                        {
                            streams.stderr_path = registers[arg1];
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '{':
                    {
                        if(line.size() == 1)
                        {
                            streams.stderr_append = !streams.stderr_append;
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '~':
                    {
                        if(line.size() == 1)
                        {
                            background = !background;
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    case '?':
                    {
                        if(line.size() == 1)
                        {
                            for(size_t i = 0; environ[i] != 0; i++)
                            {
                                terminal.print("%s\r\n", environ[i]);
                            }
                        }
                        else
                        {
                            terminal.print_error("Invalid instruction '%s'\r\n", lines[ip].c_str());
                        }
                        break;
                    }
                    default:
                    {
                        terminal.print_error("Unknown command '%s'\r\n", op.c_str());
                        break;
                    }
                }
            }
        }
    }
}
