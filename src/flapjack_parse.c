#include <flapjack_parse.h>
#include <error_print.h>
#include <flapjack_mem.h>
#include <flapjack_commands.h>
#include <flapjack_printf.h>

#define LOAD_FACTOR 0.8

typedef struct
{
    String* name;
    size_t ip;
    bool valid;
} Label;

typedef struct
{
    Label* label_array;
    size_t capacity;
    size_t len;
} LabelHashes;

static LabelHashes init_label_hashes()
{
    LabelHashes labels = (LabelHashes){.len = 0, .capacity = 0, .label_array = NULL};
    return labels;
}

static void dest_label_hashes(LabelHashes* labels)
{
    for(size_t i = 0; i < labels->len; i++)
    {
        free(labels->label_array);
        labels->len = 0;
        labels->capacity = 0;
    }
}

static size_t find_label_hashes_insert_slot(Label* array, size_t capacity, String* item)
{
    for(size_t i = 0; true; i++)
    {
        size_t index = (item->hash_val + i) % capacity;
        if(!array[index].valid)
        {
            return index;
        }
    }
}

static bool lookup_jump(const LabelHashes* labels, String* label, size_t* index)
{
    for(size_t i = 0; true; i++)
    {
        size_t hash_index = (label->hash_val + i) % labels->capacity;
        if(!labels->label_array[hash_index].valid)
        {
            return false;
        }
        else if(label == labels->label_array[hash_index].name)
        {
            *index = labels->label_array[hash_index].ip;
            return true;
        }
    }
}

static void resize_label_hashes(LabelHashes* labels, size_t new_cap)
{
    Label* next_array = realloc_array(NULL, new_cap, sizeof(Label));
    for(size_t i = 0; i < new_cap; i++)
    {
        next_array[i] = (Label){.ip = 0, .name = NULL, .valid = false};
    }
    for(size_t i = 0; i < labels->capacity; i++)
    {
        if(labels->label_array[i].valid)
        {
            next_array[find_label_hashes_insert_slot(next_array, new_cap, labels->label_array[i].name)] = labels->label_array[i];
        }
    }
    labels->capacity = new_cap;
    free(labels->label_array);
    labels->label_array = next_array;
}

static void add_jump(LabelHashes* labels, String* label, size_t ip)
{
    if(LOAD_FACTOR * labels->capacity <= (labels->len + 1))
    {
        resize_label_hashes(labels, get_new_array_capacity(labels->len + 1, labels->capacity));
    }
    size_t index;
    if(lookup_jump(labels, label, &index))
    {
        labels->label_array[index].ip = ip;
    }
    else
    {
        labels->label_array[index] = (Label){.ip = ip, .name = label, .valid = true};
    }
    labels->len++;
}

static StringArray split_line(String* text)
{
    StringArray res = init_string_array();
    String* word = get_string("", 0);
    bool in_quotes = false;
    char quote = 0;
    bool escape = false;
    for(size_t i = 0; i < text->len; i++)
    {
       if(in_quotes)
       {
           if(escape)
           {
               switch(text->msg[i])
               {
                   case 'n':
                   {
                       word = insert_str(word, '\n', word->len);
                       break;
                   }
                   case 'r':
                   {
                       word = insert_str(word, '\r', word->len);
                       break;
                   }
                   case 't':
                   {
                       word = insert_str(word, '\t', word->len);
                       break;
                   }
                   case '\\':
                   {
                       word = insert_str(word, '\\', word->len);
                       break;
                   }
                   case '\"':
                   {
                       word = insert_str(word, '\"', word->len);
                       break;
                   }
                   case '\'':
                   {
                       word = insert_str(word, '\'', word->len);
                       break;
                   }
                   default:
                   {
                       word = insert_str(word, '\\', word->len);
                       word = insert_str(word, text->msg[i], word->len);
                       break;
                   }
               }
               escape = false;
           }
           else
           {
               if(text->msg[i] == quote)
               {
                   quote = 0;
                   in_quotes = false;
               }
               else if(text->msg[i] == '\\')
               {
                   escape = true;
               }
               else
               {
                   word = insert_str(word, text->msg[i], word->len);
               }
           }
       }
       else
       {
           switch(text->msg[i])
           {
               case '\'':
               case '\"':
               {
                   quote = text->msg[i];
                   in_quotes = true;
                   break;
               }
               case ' ':
               {
                   if(word->len > 0)
                   {
                       string_array_add_string(&res, word);
                       word = get_string("", 0);
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
                   word = insert_str(word, text->msg[i], word->len);
                   break;
               }
           }
       }
    }
    if(word->len > 0)
    {
        string_array_add_string(&res, word);
    }
    return res;
}

static bool check_reg_arg(String* arg)
{
    if(arg->len != 1)
    {
        return false;
    }
    char val = arg->msg[0];
    val -= '0';
    if(val <= '9')
    {
        return true;
    }
    return false;
}

static size_t get_reg_arg(String* arg)
{
    return arg->msg[0] - '0';
}

static bool check_num_args(StringArray* args, size_t expected)
{
    if(args->len != expected)
    {
        return false;
    }
    return true;
}

static StringArray get_command_args(String** registers, const StringArray* line)
{
    StringArray res = init_string_array();
    for(size_t i = 0; i < line->len - 1; i++)
    {
        string_array_add_string(&res, registers[get_reg_arg(line->elements[i])]);    
    }
    return res;
}

#define NUM_REGISTERS 10
String* registers[NUM_REGISTERS];
String* child_stdin;
String* child_stdout;
bool stdout_write;

void init_parser()
{
    for(size_t i = 0; i < NUM_REGISTERS; i++)
    {
        registers[i] = get_string("", 0);
        add_string_reference(registers[i]);
    }
    child_stdin = NULL;
    child_stdout = NULL;
    stdout_write = true;
}

void parse_varelse(String** current_dir, StringArray* lines, size_t ip)
{
    LabelHashes labels = init_label_hashes();
    for(size_t i = 0; i < lines->len; i++)
    {
        if(lines->elements[i]->msg[lines->elements[i]->len - 1] == '<')
        {
            StringArray line = split_line(lines->elements[i]);
            String* op = line.elements[line.len - 1]; 
            if(op->len == 1)
            {
                if(check_num_args(&line, 2))
                {
                    add_string_reference(line.elements[0]);
                    add_jump(&labels, line.elements[0], i + 1);
                }
                else
                {
                    print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                }
            }
            dest_string_array(&line);
        }
    }
    clear_string_pool();
    while(ip < lines->len)
    {
        StringArray line = split_line(lines->elements[ip]);
        if(line.len > 0)
        {
            String* op = line.elements[line.len - 1]; 
            if(op->len != 1)
            {
                print_error("Unknown command '%s'\r\n", op->msg);
                ip++;
            }
            else
            {
                switch(op->msg[0])
                {
                    case ';':
                    {
                        if(check_num_args(&line, 3) && check_reg_arg(line.elements[0]) && check_reg_arg(line.elements[1]))
                        {
                            size_t arg1 = get_reg_arg(line.elements[0]);
                            size_t arg2 = get_reg_arg(line.elements[1]);
                            remove_string_reference(registers[arg2]);
                            registers[arg2] = registers[arg1];
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case ':':
                    {
                        if(check_num_args(&line, 3) && check_reg_arg(line.elements[0]))
                        {
                            size_t arg1 = get_reg_arg(line.elements[0]);
                            remove_string_reference(registers[arg1]);
                            registers[arg1] = line.elements[1];
                            add_string_reference(registers[arg1]);
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '<':
                    {
                        ip++;
                        break;
                    }
                    case '>':
                    {
                        if(check_num_args(&line, 2) && check_reg_arg(line.elements[0]))
                        {
                            size_t arg1 = get_reg_arg(line.elements[0]);
                            String* loc = registers[arg1];
                            if(!lookup_jump(&labels, loc, &ip))
                            {
                                print_error("Invalid jump location '%s'\r\n", loc->msg);
                                ip++;
                            }
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                            ip++;
                        }
                        break;
                    }
                    case '#':
                    {
                        if(line.len < 2)
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        else
                        {
                            bool valid = true;
                            for(size_t i = 0; i < line.len - 1 && valid; i++)
                            {
                                if(!check_reg_arg(line.elements[i]))
                                {
                                    valid = false;
                                }
                            }
                            if(valid)
                            {
                                StringArray cmd_args = get_command_args(registers, &line);
                                exec_process(&cmd_args, child_stdin, child_stdout, stdout_write);
                                dest_string_array(&cmd_args);
                            }
                            else
                            {
                                print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                            }
                        }
                        ip++;
                        break;
                    }
                    case '@':
                    {
                        if(check_num_args(&line, 2) && check_reg_arg(line.elements[0]))
                        {
                            StringArray cmd_args = get_command_args(registers, &line);
                            cd_cmd(current_dir, &cmd_args);
                            dest_string_array(&cmd_args);
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '_':
                    {
                        bool valid = true;
                        for(size_t i = 0; i < line.len - 1 && valid; i++)
                        {
                            if(!check_reg_arg(line.elements[i]))
                            {
                                valid = false;
                            }
                        }
                        if(valid)
                        {
                            StringArray cmd_args = get_command_args(registers, &line);
                            dir_cmd(*current_dir, &cmd_args);
                            dest_string_array(&cmd_args);
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case ')':
                    {
                        if(line.len == 1)
                        {
                            flapjack_printf("\033[2J\033[H");
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '\\':
                    {
                        if(check_num_args(&line, 2) && check_reg_arg(line.elements[0]))
                        {
                            StringArray cmd_args = get_command_args(registers, &line);
                            flapjack_printf("%s\n\r", cmd_args.elements[0]->msg);
                            dest_string_array(&cmd_args);
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '=':
                    {
                        if(line.len == 1)
                        {
                            exit(0);
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '-':
                    {
                        if(line.len == 1)
                        {
                            flapjack_printf("Registers\r\n");
                            for(size_t i = 0; i < NUM_REGISTERS; i++)
                            {
                                flapjack_printf("\t[%zu] \'%s\'\r\n", i, registers[i]->msg);
                            }
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '(':
                    {
                        if(line.len == 1)
                        {
                            if(child_stdin != NULL)
                            {
                                remove_string_reference(child_stdin);
                            }
                            child_stdin = NULL;
                        }
                        else if(line.len == 2 && check_reg_arg(line.elements[0]))
                        {
                            if(child_stdin != NULL)
                            {
                                remove_string_reference(child_stdin);
                            }
                            child_stdin = registers[get_reg_arg(line.elements[0])];
                            add_string_reference(child_stdin);    
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case ']':
                    {
                        if(line.len == 1)
                        {
                            if(child_stdout != NULL)
                            {
                                remove_string_reference(child_stdout);
                            }
                            child_stdout = NULL;
                        }
                        else if(line.len == 2 && check_reg_arg(line.elements[0]))
                        {
                            if(child_stdout != NULL)
                            {
                                remove_string_reference(child_stdout);
                            }
                            child_stdout = registers[get_reg_arg(line.elements[0])];
                            stdout_write = true;
                            add_string_reference(child_stdout);    
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    case '}':
                    {
                        if(line.len == 1)
                        {
                            if(child_stdout != NULL)
                            {
                                remove_string_reference(child_stdout);
                            }
                            child_stdout = NULL;
                            stdout_write = false;
                        }
                        else if(line.len == 2 && check_reg_arg(line.elements[0]))
                        {
                            if(child_stdout != NULL)
                            {
                                remove_string_reference(child_stdout);
                            }
                            child_stdout = registers[get_reg_arg(line.elements[0])];
                            add_string_reference(child_stdout);
                            stdout_write = false;
                        }
                        else
                        {
                            print_error("Invalid instruction '%s'\r\n", lines->elements[ip]->msg);
                        }
                        ip++;
                        break;
                    }
                    default:
                    {
                        print_error("Unknown command '%s'\r\n", op->msg);
                        ip++;
                        break;
                    }
                }
            }
        }
        else
        {
            ip++;
        }
        dest_string_array(&line);
        clear_string_pool();
    }
}
