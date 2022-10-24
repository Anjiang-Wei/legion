#ifndef __TREE
#define __TREE
#include <memory>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <string.h>
#include <assert.h>

using namespace Legion;
using namespace Legion::Mapping;

extern std::string processor_kind_to_string(Processor::Kind kind);

template <typename T1, typename T2>
struct PairHash
{
  using VAL = std::pair<T1, T2>;
  std::size_t operator()(VAL const& pair) const noexcept
  {
    return std::hash<T1>{}(pair.first) << 1 ^ std::hash<T2>{}(pair.second);
  }
};

enum ProcessorEnum
{
    CPU,
    GPU,
    IO,
    PY,
    PROC,
    OMP,
    ALLPROC,
};

const char* ProcessorEnumName[] =
{
    "CPU",
    "GPU",
    "IO",
    "PY",
    "PROC",
    "OMP",
    "ALLPROC",
};

enum MemoryEnum
{
    SYSMEM,
    FBMEM,
    RDMEM,
    ZCMEM,
    SOCKMEM,
    ALLMEM,
};

const char* MemoryEnumName[] =
{
    "SYSMEM",
    "FBMEM",
    "RDMEM",
    "ZCMEM",
    "SOCKMEM",
    "ALLMEM",
};

enum ArgTypeEnum
{
    TASK,
    INT,
    BOOL,
    IPOINT,
    ISPACE,
    MSPACE,
};

const char* ArgTypeName[] =
{
    "TASK",
    "INT",
    "BOOL",
    "IPOINT",
    "ISPACE",
    "MSPACE",
};

enum BinOpEnum
{
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    BIGGER,
    SMALLER,
    GE,
    LE,
    EQ,
    NEQ,
    OR,
    AND,
};

const char* BinOpName[] = 
{
    "+",
    "-",
    "*",
    "/",
    "%",
    ">",
    "<",
    ">=",
    "<=",
    "==",
    "!=",
    "||",
    "&&",
};


enum APIEnum
{
    SIZE,
    SPLIT,
    MERGE,
    SWAP,
    SLICE,
    REVERSE,
    BALANCE_SPLIT,
    AUTO_SPLIT,
    GREEDY_SPLIT,
    VOLUME,
    HAS,
    LEN,
    TASKIPOINT,
    TASKISPACE,
    TASKPARENT,
    TASKPROCESSOR,
};

const char* APIName[] =
{
    "SIZE",
    "SPLIT",
    "MERGE",
    "SWAP",
    "SLICE",
    "REVERSE",
    "BALANCE_SPLIT",
    "AUTO_SPLIT",
    "GREEDY_SPLIT",
    "VOLUME",
    "HAS",
    "LEN",
    "TASKIPOINT",
    "TASKISPACE",
    "TASKPARENT",
    "TASKPROCESSOR",
};

enum NodeType
{
    ProgramType,
    StmtType,
    ProcLstType,
    ProcType,
    MemLstType,
    MemType,
    ProcCustomType,
    RegionCustomType,
    ArgTypeType,
    AssignType,
    IndexTaskMapType,
    SingleTaskMapType,
    ArgType,
    ArgLstType,
    FuncDefType,
    BinaryExprType,
    IdentifierExprType,
    IdentifierLstType,
    IntValType,
    BoolValType,
    TupleExprType,
    SliceExprType,
    FuncInvokeType,
    IndexExprType,
    NegativeExprType,
    ExclamationType,
    APIType,
    TenaryExprType,
    UnpackExprType,
    PrintType,
    PrintArgsType,
    ReturnType,
    FuncStmtsType,
    ObjectInvokeType,
    MSpaceType,
    TupleIntType,
    SetTupleIntType,
    StarExprType,
    TaskNodeType,
};

const char* NodeTypeName[] =
{
  "ProgramType",
  "StmtType",
  "ProcLstType",
  "ProcType",
  "MemLstType",
  "MemType",
  "ProcCustomType",
  "RegionCustomType",
  "ArgTypeType",
  "AssignType",
  "IndexTaskMapType",
  "SingleTaskMapType",
  "ArgType",
  "ArgLstType",
  "FuncDefType",
  "BinaryExprType",
  "IdentifierExprType",
  "IdentifierLstType",
  "IntValType",
  "BoolValType",
  "TupleExprType",
  "SliceExprType",
  "FuncInvokeType",
  "IndexExprType",
  "NegativeExprType",
  "ExclamationType",
  "APIType",
  "TenaryExprType",
  "UnpackExprType",
  "PrintType",
  "PrintArgsType",
  "ReturnType",
  "FuncStmtsType",
  "ObjectInvokeType",
  "MSpaceType",
  "TupleIntType",
  "SetTupleIntType",
  "StarExprType",
  "TaskNodeType",
};

class Node //: public std::enable_shared_from_this<Node>
{
public:
    NodeType type;
    virtual ~Node() {}
    virtual void print() {};
    virtual Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) 
    {
        std::cout << "Run method TBD:" << NodeTypeName[this->type] << std::endl;
        return NULL;
    }
    /*
    virtual std::shared_ptr<Node> getptr()
    {
        return shared_from_this();
    }*/
};

class ExprNode : public Node
{
public:
    
    ExprNode() {}
    virtual ~ExprNode() {};
    virtual void print() = 0;
    virtual Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        std::cout << "Run method TBD" << NodeTypeName[this->type] << std::endl;
        return NULL;
    }
};

class StmtNode : public Node
{
public:
    StmtNode() { type = StmtType; }
};

class ProgramNode : public Node
{
// Program: Stmt+  
public:
    std::vector<StmtNode*> stmt_list;

    ProgramNode() { type = ProgramType; }
    void print();
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        for (size_t i = 0; i < stmt_list.size(); i++)
        {
            stmt_list[i]->run(local_symbol, local_temps);
        }
        return NULL;
    }
};

ProgramNode* root;
std::unordered_map<std::string, Node*> global_symbol;
void push_local_symbol_with_top_merge(std::stack<std::unordered_map<std::string, Node*>>& local_symbol,
                                      std::unordered_map<std::string, Node*> x);
void local_temps_pop(std::vector<Node*>& local_temps); // free all the nodes in local_temps


class FuncStmtsNode : public Node
{
public:
    std::vector<StmtNode*> stmtlst;
    FuncStmtsNode()
    {
        type = FuncStmtsType;
    }
    void print()
    {
        printf("FuncStmtsNode\n");
        for (size_t i = 0; i < stmtlst.size(); i++)
        {
            stmtlst[i]->print();
        }
    }
};

class APINode : public Node
{
public:
    APIEnum api;
    APINode(APIEnum x)
    {
        type = APIType;
        api = x;
    }
    void print() { printf("APINode %s\n", APIName[api]); }
};

class ProcNode : public ExprNode
{
public:
    ProcessorEnum proc_type;

    ProcNode(ProcessorEnum x) { type = ProcType; proc_type = x;}
    void print() { printf("ProcNode: %s\n", ProcessorEnumName[proc_type]); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
};

class MemNode : public ExprNode
{
public:
    MemoryEnum mem_type;

    MemNode(MemoryEnum x) { type = MemType; mem_type = x; }
    void print() { printf("MemNode: %s\n", MemoryEnumName[mem_type]); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
};

class ProcLstNode : public Node
{
public:
    std::vector<ProcessorEnum> proc_type_lst;

    ProcLstNode() { type = ProcLstType; }
    void print();
};

class MemLstNode : public Node
{
public:
    std::vector<MemoryEnum> mem_type_lst;

    MemLstNode() { type = MemLstType; }
    void print();
};

class ProcCustomNode : public StmtNode
{
public:
    std::string taskname;
    std::vector<ProcessorEnum> proc_types;

    ProcCustomNode(const char* x, const ProcLstNode* y)
    {
        type = ProcCustomType;
        taskname = std::string(x);
        proc_types = y->proc_type_lst;
    }
    void print() { printf("ProcCustomNode %s %s\n", taskname.c_str(), ProcessorEnumName[proc_types[0]]); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class RegionCustomNode : public StmtNode
{
public:
    std::string taskname;
    std::string region_name;
    ProcessorEnum processor_type;
    std::vector<MemoryEnum> mem_types;

    RegionCustomNode(const char* x, const char* y, ProcNode* z, const MemLstNode* a)
    {
        type = RegionCustomType;
        taskname = std::string(x);
        region_name = std::string(y);
        processor_type = z->proc_type;
        mem_types = a->mem_type_lst;
    }
    void print()
    {
        printf("RegionCustomNode %s %s %s: %s as first\n", 
            taskname.c_str(), region_name.c_str(), ProcessorEnumName[processor_type],
            MemoryEnumName[mem_types[0]]);
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};


class ArgTypeNode : public Node
{
public:
    ArgTypeEnum argtype;

    ArgTypeNode(ArgTypeEnum x) { type = ArgTypeType; argtype = x; }
    void print() { printf("ArgTypeNode %s\n", ArgTypeName[argtype]); }
};

class AssignNode : public StmtNode
{
public:
    std::string var_name;
    Node* right_node;

    AssignNode(const char* x, Node* y) { type = AssignType; var_name = std::string(x); right_node = y; }
    void print()
    {
        printf("AssignNode: %s\n", var_name.c_str());
        right_node->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        Node* simplified = right_node->run(local_symbol, local_temps);
        if (!(simplified->type == IntValType || simplified->type == BoolValType ||\
            simplified->type == MSpaceType || simplified->type == TupleExprType ||\
            simplified->type == TupleIntType || simplified->type == TaskNodeType))
        {
            std::cout << "Cannot Assign " << NodeTypeName[simplified->type] << std::endl;
            assert(false);
        }
        // todo: pass in and modify env instead 
        if (local_symbol.size() >= 1)
        {
            local_symbol.top().insert({var_name, simplified});
        }
        else
        {
            if (global_symbol.count(var_name) > 0)
            {
                printf("AssignNode multiple %s cause conflicts in global_symbol!\n", var_name.c_str());
                assert(false);
            }
            assert(local_symbol.size() == 0);
            // only hit this line when building the AST
            global_symbol.insert({var_name, simplified});
        }
        // printf("AssignNode: %s inserted\n", var_name.c_str());
        // simplified->print();
        return NULL;
    }
};


class BinaryExprNode : public ExprNode
{
public:
    ExprNode* left;
    ExprNode* right;
    BinOpEnum op;
    BinaryExprNode(ExprNode* x, BinOpEnum a, ExprNode* y)
    {
        type = BinaryExprType;
        left = x; right = y; op = a;
    }
    void print()
    {
        printf("BinaryExprNode, %s\n", BinOpName[op]);
        printf("left\n"); left->print();
        printf("right\n"); right->print();
        printf("---------\n");
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class IdentifierLstNode : public Node
{
public:
    std::vector<std::string> idlst;
    IdentifierLstNode(const char* x, const char* y)
    {
        type = IdentifierLstType;
        idlst.push_back(std::string(x));
        idlst.push_back(std::string(y));
    }
    void append(const char* x)
    {
        idlst.push_back(std::string(x));
    }
};

class IdentifierExprNode : public ExprNode
{
public:
    std::string name;

    IdentifierExprNode(const char* x)
    {
        type = IdentifierExprType;
        name = std::string(x);
    }
    void print()
    {
        printf("IdentifierExprNode %s\n", name.c_str());
    }
    // todo: implement Node* run(env)
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        if (local_symbol.size() > 0)
        {
            if (local_symbol.top().count(name) > 0)
            {
                return local_symbol.top().at(name)->run(local_symbol, local_temps);
            }
        }
        if (global_symbol.count(name) == 0)
        {
            std::cout << name << " not found" << std::endl;
            assert(false);
        }
        return global_symbol.at(name)->run(local_symbol, local_temps);
    }
};

class IntValNode : public ExprNode
{
public:
    int intval;

    IntValNode(int x) { type = IntValType; intval = x; }
    void print()
    {
        printf("%d", intval);
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        return this;
    }
    IntValNode* negate(std::vector<Node*>& local_temps)
    {
        IntValNode* tmpnode = new IntValNode(-intval);
        local_temps.push_back(tmpnode);
        return tmpnode;
    }
    Node* binary_op(IntValNode* rt, BinOpEnum op, std::vector<Node*>& local_temps);
};

class TupleIntNode : public ExprNode
{
public:
    std::vector<int> tupleint;
    ProcessorEnum final_proc; // validate mapping decision against dynamic machine model
    TupleIntNode(std::vector<int> x)
        { type = TupleIntType; tupleint = x; final_proc = ALLPROC; }
    TupleIntNode(std::vector<int> x, ProcessorEnum proc)
        { type = TupleIntType; tupleint = x; final_proc = proc; }
    void print() 
    {
        for (size_t i = 0; i < tupleint.size(); i++) 
        {
            printf("%d", tupleint[i]);
            if (i != tupleint.size() - 1)
            {
                printf(",");
            }
        }
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
    TupleIntNode* negate(std::vector<Node*>& local_temps);
    TupleIntNode* slice(int a, int b, std::vector<Node*>& local_temps);
    TupleIntNode* binary_op(TupleIntNode* rt, BinOpEnum op, std::vector<Node*>& local_temps);
    IntValNode* at(int x, std::vector<Node*>& local_temps);
    IntValNode* at(IntValNode* x, std::vector<Node*>& local_temps);
    IntValNode* volume(std::vector<Node*>& local_temps);
    IntValNode* len(std::vector<Node*>& local_temps);
};

class SetTupleIntNode : public ExprNode
{
public:
    std::vector<std::vector<int>> tupletupleint;
    ProcessorEnum final_proc; // validate mapping decision against dynamic machine model
    SetTupleIntNode(std::vector<std::vector<int>> x)
        { type = SetTupleIntType; tupletupleint = x; final_proc = ALLPROC; }
    SetTupleIntNode(std::vector<std::vector<int>> x, ProcessorEnum proc)
        { type = SetTupleIntType; tupletupleint = x; final_proc = proc; }
    void print()
    {
        for (size_t kk = 0; kk < tupletupleint.size(); kk++) 
        {
            for (int i = 0; i < tupletupleint[kk].size(); i++)
            {
                printf("%d", tupletupleint[kk][i]);
                if (i != tupletupleint[kk].size() - 1)
                {
                    printf(",");
                }
            }
            printf("\n");
        }
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
};

class BoolValNode : public ExprNode
{
public:
    bool boolval;

    BoolValNode(bool x) { type = BoolValType; boolval = x; }
    void print()
    {
        printf("BoolValNode: %s\n", boolval ? "true" : "false");
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
    BoolValNode* binary_op(BoolValNode* rt, BinOpEnum op, std::vector<Node*>& local_temps);
};


class SingleTaskMapNode : public StmtNode
{
    std::vector<std::string> task_name;
    std::string func_name;
public:
    SingleTaskMapNode(const char* x, const char* y)
    {
        type = SingleTaskMapType;
        task_name.push_back(std::string(x));
        func_name = std::string(y);
    }
    SingleTaskMapNode(IdentifierLstNode* x, const char* y)
    {
        type = SingleTaskMapType;
        task_name = x->idlst;
        func_name= std::string(y);
    }
    void print() { printf("SingleTaskMapNode (%s,...), %s\n", task_name[0].c_str(), func_name.c_str()); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};


class IndexTaskMapNode : public StmtNode
{
    std::vector<std::string> task_name;
    std::string func_name;

public:
    IndexTaskMapNode(const char* x, const char* y)
    {
        type = IndexTaskMapType;
        task_name.push_back(std::string(x));
        func_name = std::string(y);
    }
    IndexTaskMapNode(IdentifierLstNode* x, const char* y)
    {
        type = IndexTaskMapType;
        task_name = x->idlst;
        func_name= std::string(y);
    }
    void print() { printf("IndexTaskMapNode (%s,...), %s\n", task_name[0].c_str(), func_name.c_str()); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class ArgNode : public Node
{
public:
    ArgTypeEnum argtype;
    std::string argname;
    ArgNode(ArgTypeNode* x, const char* y)
    {
        type = ArgType;
        argtype = x->argtype;
        argname = std::string(y);
    }
};

class ArgLstNode : public Node
{
public:
    std::vector<ArgNode*> arg_lst;
    ArgLstNode()
    {
        type = ArgLstType;
    }
    void print()
    {
        printf("ArgLstNode: %ld\n", arg_lst.size());
        for (size_t i = 0; i < arg_lst.size(); i++)
        {
            printf("Arg %ld: %s %s\n", i, ArgTypeName[arg_lst[i]->argtype], arg_lst[i]->argname.c_str());
        }
    }
};

class FuncDefNode : public StmtNode
{
public:
    std::string func_name;
    ArgLstNode* func_args;
    FuncStmtsNode* func_stmts;
    FuncDefNode(const char* x, ArgLstNode* y, FuncStmtsNode* z)
    {
        type = FuncDefType;
        func_name = std::string(x);
        func_args = y;
        func_stmts = z;
    }
    void print()
    {
        printf("FuncDefNode %s\n", func_name.c_str());
        func_args->print();
        func_stmts->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        if (global_symbol.count(func_name) == 0)
        {
            assert(local_symbol.size() == 0);
            // only hit this line when building the AST
            global_symbol.insert({func_name, this});
        }
        return this;
    }
    Node* invoked(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};


class TupleExprNode : public ExprNode
{
public:
    std::vector<Node*> exprlst;
    TupleExprNode() { type = TupleExprType; }
    TupleExprNode(Node* a)
    {
        type = TupleExprType;
        exprlst.push_back(a);
    }
    TupleExprNode(std::vector<Node*> v)
    {
        type = TupleExprType;
        exprlst = v;
    }
    void print()
    {
        for (size_t i = 0; i < exprlst.size(); i++)
        {
            exprlst[i]->print();
            if (i != exprlst.size() - 1)
            {
                printf(",");
            }
        }
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
    ExprNode* Convert2TupleInt() // for parser.y
    {
        std::vector<Node*> no_effect;
        return this->Convert2TupleInt(no_effect);
    }
    ExprNode* Convert2TupleInt(std::vector<Node*>& local_temps, bool allow_star=false); // if all nodes in exprlst are IntValNode(IntValType), then can be converted
    TupleExprNode* negate(std::vector<Node*>& local_temps);
    TupleExprNode* slice(int a, int b, std::vector<Node*>& local_temps);
    TupleExprNode* binary_op(TupleExprNode* right_op, BinOpEnum op, std::vector<Node*>& local_temps);
    Node* at(int x, std::vector<Node*>& local_temps);
    Node* at(IntValNode* x, std::vector<Node*>& local_temps);
    IntValNode* one_int_only();
};

class StarExprNode : public ExprNode
{
public:
    StarExprNode() { type = StarExprType; }
    void print() { printf("*"); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
};

class SliceExprNode : public ExprNode
{
public:
    ExprNode* left;
    ExprNode* right;
    SliceExprNode(ExprNode* l, ExprNode* r)
    {
        type = SliceExprType;
        left = l;
        right = r;
    }
    void print()
    {
        printf("SliceExprNode\n");
        left->print();
        right->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        return this;
    }
};

class ForTupleExprNode : public ExprNode
{
public:
    ExprNode* expr;
    std::string identifier;
    ExprNode* range;
    ForTupleExprNode(ExprNode* x, const char* y, ExprNode* z)
    {
        expr = x;
        identifier = std::string(y);
        if ((z->type != TupleExprType && z->type != TupleIntType))
        {
            std::cout << "For-comprehension must itererate over TupleExpr or TupleInt" << std::endl;
            assert(false);
        }
        range = z;
    }
    void print()
    {
        printf("ForTupleExprNode\n");
        expr->print();
        std::cout << identifier << std::endl;
        range->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class FuncInvokeNode : public ExprNode
{
public:
    ExprNode* func_node;
    TupleExprNode* args_node;
    FuncInvokeNode(ExprNode* x, TupleExprNode* y)
    {
        type = FuncInvokeType;
        func_node = x;
        args_node = y;
    }
    void print()
    {
        printf("FuncInvokeNode\n");
        func_node->print();
        args_node->print();
    }
    // Node* broadcast(); // deal with !()
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class ObjectInvokeNode : public ExprNode
{
public:
    ExprNode* obj;
    APIEnum api;
    ObjectInvokeNode(ExprNode* x, APINode* y)
    {
        type = ObjectInvokeType;
        obj = x;
        api = y->api;
    }
    void print()
    {
        printf("ObjectInvokeNode\n");
        obj->print();
        printf("%s\n", APIName[api]);
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class IndexExprNode : public ExprNode
{
public:
    ExprNode* tuple;
    ExprNode* index;
    IndexExprNode(ExprNode* x, ExprNode* y)
    {
        type = IndexExprType;
        tuple = x;
        index = y;
    }
    IndexExprNode(ExprNode* x, TupleExprNode* y)
    {
        type = IndexExprType;
        tuple = x; // must be a machine model (identifier)
        index = y;
    }
    void print()
    {
        printf("IndexExprNode\n");
        tuple->print();
        index->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class NegativeExprNode : public ExprNode
{
public:
    ExprNode* neg;
    NegativeExprNode(ExprNode* x)
    {
        type = NegativeExprType;
        neg = x;
    }
    void print()
    {
        printf("NegativeExprNode\n");
        neg->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

/*
class ExclamationNode : public ExprNode
{
public:
    ExprNode* expr;
    ExclamationNode(ExprNode* x)
    {
        type = ExclamationType;
        expr = x;
    }
    void print()
    {
        printf("ExclamationNode\n");
        expr->print();
    }
};
*/

class TenaryExprNode : public ExprNode
{
public:
    ExprNode* bool_exp;
    ExprNode* true_exp;
    ExprNode* false_expr;
    TenaryExprNode(ExprNode* x, ExprNode* y, ExprNode* z)
    {
        type = TenaryExprType;
        bool_exp = x; true_exp = y; false_expr = z;
    }
    void print()
    {
        printf("TenaryExprNode\n");
        bool_exp->print();
        true_exp->print();
        false_expr->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        Node* simplified_node = bool_exp->run(local_symbol, local_temps);
        assert(simplified_node->type == BoolValType);
        BoolValNode* bool_node = (BoolValNode*) simplified_node;
        if (bool_node->boolval)
        {
            return true_exp->run(local_symbol, local_temps);
        }
        else
        {
            return false_expr->run(local_symbol, local_temps);
        }
    }
};

class UnpackExprNode : public ExprNode
{
public:
    ExprNode* expr;

    UnpackExprNode(ExprNode* x) { type = UnpackExprType; expr = x; }
    void print()
    {
        printf("UnpackExprNode\n");
        expr->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        Node* res = expr->run(local_symbol, local_temps);
        assert(res->type == TupleExprType || res->type == TupleIntType || res->type == SetTupleIntType);
        UnpackExprNode* tmpnode = new UnpackExprNode((ExprNode*) res);
        local_temps.push_back(tmpnode);
        return tmpnode;
    }
};


class PrintArgsNode : public Node
{
public:
    std::vector<ExprNode*> printargs;

    PrintArgsNode() { type = PrintArgsType; }
};

class PrintNode : public StmtNode
{
public:
    std::string format_string;
    std::vector<ExprNode*> printargs;
    PrintNode(const char* x, PrintArgsNode* y)
    {
        type = PrintType;
        format_string = std::string(x);
        printargs = y->printargs;
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
    void print()
    {
        printf("PrintNode\n");
    }
};

class ReturnNode : public StmtNode
{
public:
    ExprNode* ret_expr;
    ReturnNode(ExprNode* x)
    {
        type = ReturnType;
        ret_expr = x;
    }
    void print() { printf("ReturnNode\n"); ret_expr->print(); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps)
    {
        Node* result = ret_expr->run(local_symbol, local_temps);
        assert(result->type == TupleExprType || result->type == TupleIntType || \
            result->type == SetTupleIntType || \
            result->type == IntValType || result->type == BoolValType);
        // std::cout << "return Node executed" << std::endl;
        // result->print();
        return result;
    }
};


class ConstraintsNode : public Node
{
public:
    bool reverse;
    bool aos;
    bool compact;
    bool exact;
    bool align;
    BinOpEnum align_op;
    int align_int;
    ConstraintsNode()
    {
        reverse = false;
        aos = true;
        compact = false;
        exact = false;
        align = false;
        align_op = PLUS;
        align_int = 0;
    }
    void update(const char* x);
    void update(BinOpEnum x, int y);
    void print() { std::cout << "ConstraintsNode" << std::endl; }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return NULL; }
};

class LayoutCustomNode : public StmtNode
{
    //Layout *taskname *region_name *memory_type
public:
    std::string task_name;
    std::string region_name;
    MemoryEnum mem_type;
    ConstraintsNode* constraint;
    LayoutCustomNode(const char* x0, const char* x1, MemNode* x2, ConstraintsNode* x3)
    {
        task_name = std::string(x0);
        region_name = std::string(x1);
        mem_type = x2->mem_type;
        constraint = x3;
    }
    void print() { printf("LayoutCustomNode\n"); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class MemoryCollectNode : public StmtNode
{
    // CollectMemory $task_name $region_name
public:
    std::string task_name;
    std::string region_name;
    MemoryCollectNode(const char* x0, const char* x1)
    {
        task_name = std::string(x0);
        region_name = std::string(x1);
    }
    void print() { printf("MemoryCollectNode %s %s", task_name.c_str(), region_name.c_str()); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class InstanceLimitNode : public StmtNode
{
    // InstanceLimit Foo CPU 5
public:
    std::string task_name;
    ProcessorEnum proc_type;
    int num;
    InstanceLimitNode(const char* task_name_, ProcNode* proc, int num_)
    {
        task_name = std::string(task_name_);
        proc_type = proc->proc_type;
        num = num_;
    }
    void print() { printf("InstanceLimitNode\n"); }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps);
};

class MSpace;
class NSMapper;

class TaskNode : public Node
{
public:
    // if true, use task_name, ipoint, ispace; if false, use task_obj and mapper
    bool index_launch;

    std::string task_name;
    TupleIntNode* ipoint;
    TupleIntNode* ispace;

    const Task* task_obj;
    const NSMapper* mapper;

    // for single task launch, initialize by Legion Task Object
    TaskNode(const Task* legion_task, const NSMapper* legion_mapper)
    {
        type = TaskNodeType;
        index_launch = false;
        task_obj = legion_task;
        mapper = legion_mapper;
    }
    // for index launch, initialize by point and space
    TaskNode(std::string name, std::vector<int> point, std::vector<int> space, std::vector<Node*>& local_temps)
    {
        type = TaskNodeType;
        index_launch = true;
        task_name = name;
        ipoint = new TupleIntNode(point);
        ispace = new TupleIntNode(space);
        local_temps.push_back(ipoint);
        local_temps.push_back(ispace);
    }
    TupleIntNode* get_point();
    TupleIntNode* get_space();
    TaskNode* get_parent(std::vector<Node*>& local_temps);
    std::vector<int> get_proc_coordinate_from_Legion();
    void print()
    {
        printf(index_launch ? " is index launch\n" : " not index launch\n");
        printf("ipoint:"); if (ipoint != NULL) ipoint->print();
        printf("ispace:"); if (ispace != NULL) ispace->print();
    }
    Node* run(std::stack<std::unordered_map<std::string, Node*>>& local_symbol, std::vector<Node*>& local_temps) { return this; }
};

class Tree2Legion
{
public:
    Tree2Legion() {}
    Tree2Legion(std::string filename);
    static std::unordered_map<std::string, std::vector<Processor::Kind>> task_policies;

    using HashFn1 = PairHash<std::string, std::string>;
    static std::unordered_map<std::pair<std::string, std::string>, 
        std::unordered_map<Processor::Kind, std::vector<Memory::Kind>>, HashFn1> region_policies;
    
    static std::unordered_map<std::pair<std::string, std::string>,
        std::unordered_map<Memory::Kind, ConstraintsNode*>, HashFn1> 
        layout_constraints;
    
    static std::unordered_set<std::pair<std::string, std::string>, HashFn1> memory_collect;
    
    static std::unordered_map<std::string, std::unordered_map<Processor::Kind, int>> task2limit;
    
    // static std::unordered_map<std::string, MSpace*> task2mspace;
    static std::unordered_map<std::string, FuncDefNode*> task2func;

    bool should_fall_back(std::string task_name)
    {
        if (task2func.count(task_name) > 0)
        {
            return false;
        }
        if (task2func.count("*") > 0)
        {
            return false;
        }
        // std::cout << task_name << " will fallback for sharding/slicing, warning!" << std::endl;
        return true;
    }

    bool should_collect_memory(std::string task_name, std::vector<std::string> region_name)
    {
        if (memory_collect.size() == 0)
        {
            return false;
        }
        if (memory_collect.count({"*", "*"}) > 0)
        {
            return true;
        }
        if (memory_collect.count({task_name, "*"}) > 0)
        {
            return true;
        }
        for (auto &str: region_name)
        {
            if (memory_collect.count({task_name, str}) > 0)
                return true;
            if (memory_collect.count({"*", str}) > 0)
                return true;
        }
        return false;
    }
    
    void print()
    {
        std::cout << "I am invoked!" << std::endl;
    }

    std::vector<std::vector<int>> runsingle(const Task* task, const NSMapper* mapper);
    std::vector<std::vector<int>> runindex(const Task* task);
    std::vector<std::vector<int>> runindex(std::string task, const std::vector<int>& x,
                         const std::vector<int>& point_space, Processor::Kind proc_kind = Processor::NO_KIND);
    std::vector<Memory::Kind> query_memory_policy(std::string task_name, std::string region_name, Processor::Kind proc_kind)
    {
        std::pair<std::string, std::string> key = {task_name, region_name};
        if (region_policies.count(key) > 0)
        {
            std::unordered_map<Processor::Kind, std::vector<Memory::Kind> > value = region_policies.at(key);
            if (value.count(proc_kind) > 0)
            {
                return value.at(proc_kind);
            }
            if (value.count(Processor::NO_KIND) > 0)
            {
                return value.at(Processor::NO_KIND);
            }
        }
        return {};
    }
    std::vector<Memory::Kind> query_memory_list(std::string task_name, std::vector<std::string> region_names, Processor::Kind proc_kind)
    {
        // region_names: no "*" included; will need to consider "*"
        std::vector<Memory::Kind> res;
        // exact match first
        for (auto &region_name : region_names)
        {
            std::vector<Memory::Kind> to_append = query_memory_policy(task_name, region_name, proc_kind);
            res.insert(res.end(), to_append.begin(), to_append.end());
        }
        // task_name *
        std::vector<Memory::Kind> to_append2 = query_memory_policy(task_name, "*", proc_kind);
        res.insert(res.end(), to_append2.begin(), to_append2.end());
        // * region_name
        for (auto &region_name : region_names)
        {
            std::vector<Memory::Kind> to_append3 = query_memory_policy("*", region_name, proc_kind);
            res.insert(res.end(), to_append3.begin(), to_append3.end());
        }
        // * *
        std::vector<Memory::Kind> to_append4 = query_memory_policy("*", "*", proc_kind);
        res.insert(res.end(), to_append4.begin(), to_append4.end());
        return res;
    }
    ConstraintsNode* query_constraint_one_region(const std::string &task_name, const std::string &region_name,
                                                 const Memory::Kind &mem_kind)
    {
        std::pair<std::string, std::string> key = {task_name, region_name};
        if (layout_constraints.count(key) > 0)
        {
            std::unordered_map<Memory::Kind, ConstraintsNode*> value = layout_constraints.at(key);
            if (value.count(mem_kind) > 0)
            {
                return value.at(mem_kind);
            }
            if (value.count(Memory::NO_MEMKIND) > 0)
            {
                return value.at(Memory::NO_MEMKIND);
            }
        }
        return NULL;
    }
    ConstraintsNode* query_constraint(const std::string &task_name, const std::vector<std::string> &region_names,
                                      const Memory::Kind &mem_kind)
    {
        // exact match first
        for (auto &region_name : region_names)
        {
            ConstraintsNode* res1 = query_constraint_one_region(task_name, region_name, mem_kind);
            if (res1 != NULL)
            {
                return res1;
            }
        }
        ConstraintsNode* res2 = query_constraint_one_region(task_name, "*", mem_kind);
        if (res2 != NULL)
        {
            return res2;
        }
        for (auto &region_name : region_names)
        {
            ConstraintsNode* res3 = query_constraint_one_region("*", region_name, mem_kind);
            if (res3 != NULL)
            {
                return res3;
            }
        }
        ConstraintsNode* res4 = query_constraint_one_region("*", "*", mem_kind);
        if (res4 != NULL)
        {
            return res4;
        }
        return NULL;
    }
    int query_max_instance(std::string task_name, Processor::Kind proc_kind)
    {
        if (task2limit.count(task_name) > 0)
        {
            std::unordered_map<Processor::Kind, int> kind_int = task2limit.at(task_name);
            if (kind_int.count(proc_kind) > 0)
            {
                int res = kind_int.at(proc_kind);
                assert(res > 0);
                return res;
            }
            return 0;
        }
        return 0;
    }
};

std::unordered_map<std::string, std::vector<Processor::Kind>> Tree2Legion::task_policies;
using HashFn1 = PairHash<std::string, std::string>;
std::unordered_map<std::pair<std::string, std::string>, 
    std::unordered_map<Processor::Kind, std::vector<Memory::Kind> >, HashFn1>
    Tree2Legion::region_policies;

std::unordered_map<std::pair<std::string, std::string>,
    std::unordered_map<Memory::Kind, ConstraintsNode*>, HashFn1>
    Tree2Legion::layout_constraints;
std::unordered_map<std::string, std::unordered_map<Processor::Kind, int>> Tree2Legion::task2limit;
// std::unordered_map<std::string, MSpace*> Tree2Legion::task2mspace;
std::unordered_map<std::string, FuncDefNode*> Tree2Legion::task2func;
std::unordered_set<std::pair<std::string, std::string>, HashFn1> Tree2Legion::memory_collect;
#endif
