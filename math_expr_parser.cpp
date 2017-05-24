#include "math_expr_parser.h"

void MathExprParser::m_make_rpn() {
	/*
		Converts infit notation to reverse polish notation (RPN).
	*/

	if(m_inputExpr.empty() || m_inputExpr == " ") throw BAD_INIT();

	// necessary containers
	std::stack<std::string> operatorStack;
	std::queue<std::string> outputQueue;

	// clear whitespaces from the input expression
	std::istringstream iss(m_inputExpr);
	std::string inputExprNoWS;
	std::string token;

	while(iss >> token) inputExprNoWS.append(token);
	//

	auto it = inputExprNoWS.cbegin();
	auto itBegin = inputExprNoWS.cbegin();

	while(it != inputExprNoWS.cend()) {
		if(*it == '\'') {
			// ' is the variable identifier character.
			// pass if ' is encountered.
			it++;
			continue;
		}

		if(m_isOperator(it, itBegin)) {
			// pop all operators on the top of the operator stack with greater
			// precedence than (or equal to) this operator and put them in the 
			// output queue
			// Note: Functions have higher precedence than all operators
			while(
				!operatorStack.empty() && 
					(m_precendence(operatorStack.top()) >= m_precendence(*it))
				) {
				std::string op = operatorStack.top();
				outputQueue.push(op);
				operatorStack.pop();
			}

			// finally, push this operator to the operator stack
			operatorStack.push(std::string(1, *it));
			it++;
		}
		else if(*it == '(') {
			operatorStack.push(std::string(1, *it));
			it++;
		}
		else if(*it == ')') {
			// pop all operators from the operator stack until the top element
			// is a left parenthesis and put the popped operators in the output
			// queue. Discard the right parenthesis
			while(!operatorStack.empty() && operatorStack.top() != "(") {
				std::string op = operatorStack.top();
				outputQueue.push(op);
				operatorStack.pop();
			}

			// finally, pop the left parenthesis from the operator stack and
			// discard it
			// !! check if the operator stack is empty. if it is, there's an
			//    unclosed right parenthesis
			if(operatorStack.empty()) throw UNCLOSED_RPARENTHESIS();

			operatorStack.pop();
			it++;
		}
		else if(m_isNumber(it, itBegin)) {
			std::string parsedNumber; // can be real integer or decimal number

			while(it != inputExprNoWS.cend() && m_isNumber(it, itBegin)) {
				parsedNumber.push_back(*it++);
			}

			outputQueue.push(parsedNumber);
		}
		else {
			std::string parsedFunction;

			while(it != inputExprNoWS.cend() && *it != '(') {
				parsedFunction.push_back(*it++);
			}

			operatorStack.push(parsedFunction);
		}
	}

	// Pop all items from the operator stack and push them to the output queue
	while(!operatorStack.empty()) {
		std::string op = operatorStack.top();
		outputQueue.push(op);
		operatorStack.pop();
	}

	std::string returnRPN;

	while(!outputQueue.empty()) {
		returnRPN.append(outputQueue.front());
		returnRPN.push_back(' ');
		outputQueue.pop();
	}

	// test for unclosed left parenthesis
	// If there's an unclosed left parenthesis in the beginning of the expr.,
	// it shows up as the last element of the RPN. Check for that first
	auto checkIterator = returnRPN.cend();
	if(*(checkIterator-2) == '(')
		throw UNCLOSED_LPARENTHESIS();

	// check for the rest of the RPN. In the infix notation, the unclosed left 
	// parenthesis is found after the operator/function which comes after that 
	// left parenthesis in the RPN
	std::istringstream issTestLParenthesis(returnRPN);
	std::string tokenTestLParenthesis;

	while(issTestLParenthesis >> tokenTestLParenthesis) {
		if(tokenTestLParenthesis == "(") {
			issTestLParenthesis >> tokenTestLParenthesis;
			throw UNCLOSED_LPARENTHESIS();
		}
	}

	m_rpn = returnRPN;

}

double MathExprParser::m_calc_rpn() {

/*
	Calculates the expression given in reverse polish notation, and returns the 
	result as double.
*/

	if(m_rpn.empty()) throw BAD_RPN();

	std::stack<double> numberStack;
	
	std::istringstream iss(m_rpn);
	std::string token;

	try {
		while(iss >> token) {
			if(m_isNumber(token)) {
				numberStack.push(std::stod(token));
			}
			else if(m_isOperator(token)) {
				if(numberStack.size() < 2) throw INPUT_EXPR_SYNTAX_ERROR();

				double rVal = numberStack.top();
				numberStack.pop();
				double lVal = numberStack.top();
				numberStack.pop();

				char op = token.c_str()[0];

				double operatorCalcResult = m_calc_operator(lVal, rVal, op);
				numberStack.push(operatorCalcResult);
			}
			else if(auto func = m_isFunction(token)) {
				if(numberStack.empty()) throw INPUT_EXPR_SYNTAX_ERROR();
				double val = numberStack.top();
				numberStack.pop();

				double functionCalcResult = m_calc_function(val, func);
				numberStack.push(functionCalcResult);
			}
			else {
				throw INPUT_EXPR_SYNTAX_ERROR();
			}
		}
	}
	catch(const INPUT_EXPR_SYNTAX_ERROR& e) {
		throw;
	}

	return numberStack.top();
}

double MathExprParser::calculate(vector_Var& variables) {

	for(size_t i = 0; i < variables.size(); i++) {
		std::string variableName = variables[i].name;

		// whether the variable is known or not should be checked before
		// anything else
		if(!m_variableExists(variableName))
			throw UNKNOWN_VARIABLE(variableName);
		//

		std::string modifiedVarName = "'" + variableName + "'";
		std::string variableValue = std::to_string(variables[i].value);

		size_t pos;

		while((pos = m_inputExpr.find(modifiedVarName)) != std::string::npos) {
			size_t oldVarSize = modifiedVarName.size();
			m_inputExpr.replace(pos, oldVarSize, variableValue);
		}
	}

	double result;

	try {
		m_make_rpn();
		result = m_calc_rpn();
	}
	catch(const BAD_INIT& e) {
		throw;
	}
	catch(const BAD_RPN& e) {
		throw;
	}
	
	return result;

}

bool MathExprParser::m_isOperator(const std::string::const_iterator& it, 
	const std::string::const_iterator& itBegin) const {
	/*
		it:      The string iterator iterating over the input expression.
		itBegin: The iterator pointing at the end of the input expression.
	*/

	char token = *it;

	switch(token) {
		case '+':
		case '-':
			// "-" is counted as number and not operator if:
			//	1. it is the first token in the input expression
			//	2. it comes immediately after a left parenthesis
			//	3. it comes after an operator, including "-"
			if(it == itBegin) return false;
			if(*(it-1) == '(') return false;
			if(m_isOperator(it-1, itBegin)) return false;
			return true;
		case '*':
		case '/':
		case '^':
			return true;
		default:
			return false;
	}

}

bool MathExprParser::m_isOperator(const std::string& token) const {
	/*
		Checks if token, which is taken from the number stack, is an operator.
	*/

	if(token == "+") return true;
	if(token == "-") return true;
	if(token == "*") return true;
	if(token == "/") return true;
	if(token == "^") return true;
	else return false;

}

bool MathExprParser::m_isNumber(const std::string::const_iterator& it,
	const std::string::const_iterator& itBegin) const {
	/*
	it     : the string iterator iterating over the input expression.
	itBegin: the iterator pointing at the end of the input expression.
	*/

	char token = *it;
	
	switch(token) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.': // for floating-point numbers
			return true;
		case '-':
			// "-" is counted as number and not operator if:
			//	1. it is the first token in the input expression
			//	2. it comes immediately after a left parenthesis
			//	3. it comes after an operator, including "-"
			if(it == itBegin) return true;
			if(*(it-1) == '(') return true;
			if(m_isOperator(it-1, itBegin)) return true;
			return false;
		default:
			return false;
	}

}

bool MathExprParser::m_isNumber(const std::string& token) const {
	/*
		Checks if token, which is taken from the number stack, is a number.
	*/

	try {
		std::stod(token);
	}
	catch(const std::invalid_argument& e) {
		return false;
	}

	return true;

}

bool MathExprParser::m_variableExists(const std::string& varName) const {

/*
	Checks if the varible varName exists in the input expression.
*/

	if(m_inputExpr.empty() || m_inputExpr == " ") throw BAD_INIT();

	std::string modifiedVarName = "'" + varName + "'";

	if(m_inputExpr.find(modifiedVarName) == std::string::npos)
		return false;
		
	return true;

}

FUNCTION MathExprParser::m_isFunction(const std::string& token) const {

	if(token == "LOG" || token == "log") return FUNCTION::LOG;
	if(token == "LOG10" || token == "log10") return FUNCTION::LOG10;
	if(token == "SIN" || token == "sin") return FUNCTION::SIN;
	if(token == "COS" || token == "cos") return FUNCTION::COS;
	if(token == "TAN" || token == "tan") return FUNCTION::TAN;
	if(token == "COT" || token == "cot") return FUNCTION::COT;
	if(token == "ASIN" || token == "asin") return FUNCTION::ASIN;
	if(token == "ACOS" || token == "acos") return FUNCTION::ACOS;
	if(token == "ATAN" || token == "atan") return FUNCTION::ATAN;
	if(token == "ATAN2" || token == "atan2") return FUNCTION::ATAN2;
	if(token == "ACOT" || token == "acot") return FUNCTION::ACOT;
	if(token == "DEG" || token == "deg") return FUNCTION::DEG;
	if(token == "RAD" || token == "rad") return FUNCTION::RAD;
	if(token == "SQRT" || token == "sqrt") return FUNCTION::SQRT;
	if(token == "EXP" || token == "exp") return FUNCTION::EXP;
	if(token == "ABS" || token == "abs") return FUNCTION::ABS;
	else return FUNCTION::NONE;

}

int MathExprParser::m_precendence(const char& op) const {

	switch(op) {
		case '+':
		case '-':
			return 2;
		case '*':
		case '/':
		case '%':
			return 3;
		case '^':
			return 4;
		default:
			return 1;
	}

}

int MathExprParser::m_precendence(const std::string& op) const {

	if(op == "+" || op == "-") return 2;
	if(op == "*" || op == "/" || op == "%") return 3;
	if(op == "^") return 4;
	if(m_isFunction(op)) return 5;
	else return 1;

}

double MathExprParser::m_calc_operator(double lVal, double rVal,
	char op) const {

	switch(op) {
		case '+':
			return lVal + rVal;
		case '-':
			return lVal - rVal;
		case '*':
			return lVal * rVal;
		case '/':
			return lVal / rVal;
		case '%':
			return std::fmod(lVal, rVal);
		case '^':
			return std::pow(lVal, rVal);
		default:
			throw INPUT_EXPR_SYNTAX_ERROR();
	}

}

double MathExprParser::m_calc_function(double val, FUNCTION func) const {

	switch(func) {
		case NONE:
			throw INPUT_EXPR_SYNTAX_ERROR();
		case LOG:
			return std::log(val);
		case LOG10:
			return std::log10(val);
		case SIN:
			return std::sin(val);
		case COS:
			return std::cos(val);
		case TAN:
			return std::tan(val);
		case COT:
			return 1/std::tan(val);
		case ASIN:
			return std::asin(val);
		case ACOS:
			return std::acos(val);
		case ATAN:
			return std::atan(val);
		// case ATAN2: implement later
		case ACOT:
			return std::atan(1/val);
		case DEG:
			return (val/(2*PI))*360;
		case RAD:
			return (val/360)*2*PI;
		case SQRT:
			return std::sqrt(val);
		case EXP:
			return std::exp(val);
		case ABS:
			return std::abs(val);
		default:
			throw INPUT_EXPR_SYNTAX_ERROR();
	}

}