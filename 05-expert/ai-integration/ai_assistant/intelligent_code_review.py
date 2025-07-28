#!/usr/bin/env python3
"""
Intelligent Code Review System for Embedded Projects Platform
Description: AI-powered code review with embedded systems expertise
Version: 1.0
Author: Embedded Projects Platform Team
"""

import re
import ast
import json
import sqlite3
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, asdict
from enum import Enum
from datetime import datetime
import hashlib

class ReviewSeverity(Enum):
    INFO = "info"
    WARNING = "warning" 
    ERROR = "error"
    CRITICAL = "critical"

class ReviewCategory(Enum):
    FUNCTIONALITY = "functionality"
    PERFORMANCE = "performance"
    SECURITY = "security"
    MAINTAINABILITY = "maintainability"
    EMBEDDED_BEST_PRACTICES = "embedded_best_practices"
    MEMORY_MANAGEMENT = "memory_management"
    TIMING = "timing"
    HARDWARE_INTERFACE = "hardware_interface"

@dataclass
class CodeIssue:
    issue_id: str
    category: ReviewCategory
    severity: ReviewSeverity
    title: str
    description: str
    line_number: int
    column: int
    code_snippet: str
    suggestion: str
    explanation: str
    auto_fixable: bool
    confidence: float  # 0.0-1.0

@dataclass
class PerformanceMetric:
    metric_name: str
    current_value: float
    recommended_value: float
    unit: str
    impact: str

@dataclass
class CodeReviewReport:
    file_path: str
    timestamp: datetime
    overall_score: float  # 0.0-1.0
    issues: List[CodeIssue]
    performance_metrics: List[PerformanceMetric]
    complexity_score: float
    maintainability_index: float
    embedded_compliance_score: float
    recommendations: List[str]
    auto_fixes: List[str]

class IntelligentCodeReviewer:
    def __init__(self, db_path: str = "code_review.db"):
        self.db_path = db_path
        self.init_database()
        
        # Embedded systems specific patterns and rules
        self.embedded_patterns = {
            "interrupt_handlers": [
                r"ISR\s*\(",
                r"SIGNAL\s*\(",
                r"__interrupt",
                r"interrupt\s+void"
            ],
            "hardware_registers": [
                r"\b[A-Z]{2,}[0-9]*[A-Z]*\b",  # Register names like PORTB, TCCR1A
                r"DDR[A-Z]",
                r"PIN[A-Z]",
                r"PORT[A-Z]"
            ],
            "timing_functions": [
                r"delay\s*\(",
                r"delayMicroseconds\s*\(",
                r"_delay_ms\s*\(",
                r"_delay_us\s*\("
            ],
            "memory_operations": [
                r"malloc\s*\(",
                r"free\s*\(",
                r"calloc\s*\(",
                r"realloc\s*\("
            ],
            "volatile_keywords": r"\bvolatile\b",
            "atomic_operations": [
                r"ATOMIC_BLOCK\s*\(",
                r"cli\s*\(\)",
                r"sei\s*\(\)"
            ]
        }
        
        # Performance thresholds for embedded systems
        self.performance_thresholds = {
            "max_function_lines": 50,
            "max_cyclomatic_complexity": 10,
            "max_nesting_depth": 4,
            "max_isr_lines": 20,
            "max_stack_usage": 256,  # bytes
            "max_interrupt_latency": 10,  # microseconds
            "min_code_coverage": 0.8
        }
        
        # Common embedded system vulnerabilities
        self.security_patterns = {
            "buffer_overflow": [
                r"gets\s*\(",
                r"strcpy\s*\(",
                r"sprintf\s*\(",
                r"strcat\s*\("
            ],
            "integer_overflow": [
                r"(\w+\s*[\+\-\*]\s*\w+)\s*;",  # Simple arithmetic without bounds checking
            ],
            "hardcoded_secrets": [
                r"password\s*=\s*[\"'].*[\"']",
                r"key\s*=\s*[\"'].*[\"']",
                r"secret\s*=\s*[\"'].*[\"']"
            ],
            "weak_random": [
                r"rand\s*\(\)",
                r"random\s*\("
            ]
        }

    def init_database(self):
        """Initialize database for code review history"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS code_reviews (
                review_id TEXT PRIMARY KEY,
                file_path TEXT,
                file_hash TEXT,
                review_data TEXT,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS review_metrics (
                metric_id INTEGER PRIMARY KEY AUTOINCREMENT,
                review_id TEXT,
                metric_name TEXT,
                metric_value REAL,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (review_id) REFERENCES code_reviews (review_id)
            )
        ''')
        
        conn.commit()
        conn.close()

    def review_code_file(self, file_path: str, code_content: str, 
                        project_context: Dict[str, any] = None) -> CodeReviewReport:
        """Perform comprehensive code review of a single file"""
        
        # Calculate file hash for change detection
        file_hash = hashlib.md5(code_content.encode()).hexdigest()
        
        # Initialize review report
        report = CodeReviewReport(
            file_path=file_path,
            timestamp=datetime.now(),
            overall_score=0.0,
            issues=[],
            performance_metrics=[],
            complexity_score=0.0,
            maintainability_index=0.0,
            embedded_compliance_score=0.0,
            recommendations=[],
            auto_fixes=[]
        )
        
        # Run different analysis passes
        report.issues.extend(self._analyze_functionality(code_content))
        report.issues.extend(self._analyze_embedded_practices(code_content))
        report.issues.extend(self._analyze_performance(code_content))
        report.issues.extend(self._analyze_security(code_content))
        report.issues.extend(self._analyze_memory_management(code_content))
        report.issues.extend(self._analyze_timing_constraints(code_content))
        report.issues.extend(self._analyze_hardware_interface(code_content))
        
        # Calculate metrics
        report.complexity_score = self._calculate_complexity_score(code_content)
        report.maintainability_index = self._calculate_maintainability_index(code_content)
        report.embedded_compliance_score = self._calculate_embedded_compliance(code_content)
        report.performance_metrics = self._analyze_performance_metrics(code_content)
        
        # Generate recommendations
        report.recommendations = self._generate_recommendations(report.issues, code_content)
        report.auto_fixes = self._generate_auto_fixes(report.issues)
        
        # Calculate overall score
        report.overall_score = self._calculate_overall_score(report)
        
        # Save review to database
        self._save_review_report(report, file_hash)
        
        return report

    def _analyze_functionality(self, code: str) -> List[CodeIssue]:
        """Analyze functional correctness issues"""
        issues = []
        lines = code.split('\n')
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Check for infinite loops without break conditions
            if re.search(r'while\s*\(\s*1\s*\)|while\s*\(\s*true\s*\)', line_stripped):
                # Look for break statements in the next few lines
                has_break = any('break' in lines[j] for j in range(i, min(i+10, len(lines))))
                if not has_break:
                    issues.append(CodeIssue(
                        issue_id=f"func_{i}_infinite_loop",
                        category=ReviewCategory.FUNCTIONALITY,
                        severity=ReviewSeverity.WARNING,
                        title="Potential infinite loop",
                        description="Infinite loop detected without visible break condition",
                        line_number=i,
                        column=line.find('while'),
                        code_snippet=line_stripped,
                        suggestion="Add a break condition or timeout mechanism",
                        explanation="Infinite loops can cause system hang in embedded systems",
                        auto_fixable=False,
                        confidence=0.8
                    ))
            
            # Check for missing return statements in non-void functions
            if re.search(r'(int|float|char|long|short)\s+\w+\s*\([^)]*\)\s*{', line_stripped):
                # This is a function definition - check if it has return statement
                func_end = self._find_function_end(lines, i-1)
                func_body = '\n'.join(lines[i:func_end])
                if 'return' not in func_body:
                    issues.append(CodeIssue(
                        issue_id=f"func_{i}_missing_return",
                        category=ReviewCategory.FUNCTIONALITY,
                        severity=ReviewSeverity.ERROR,
                        title="Missing return statement",
                        description="Non-void function missing return statement",
                        line_number=i,
                        column=0,
                        code_snippet=line_stripped,
                        suggestion="Add appropriate return statement",
                        explanation="Missing return statements can cause undefined behavior",
                        auto_fixable=False,
                        confidence=0.9
                    ))
            
            # Check for uninitialized variables
            var_declarations = re.findall(r'(int|float|char|bool)\s+(\w+)\s*;', line_stripped)
            for var_type, var_name in var_declarations:
                # Check if variable is used before assignment
                subsequent_lines = lines[i:]
                first_use = next((j+i for j, l in enumerate(subsequent_lines) 
                                if var_name in l and '=' not in l.split(var_name)[0]), None)
                first_assignment = next((j+i for j, l in enumerate(subsequent_lines) 
                                       if f"{var_name} =" in l or f"{var_name}=" in l), None)
                
                if first_use and (not first_assignment or first_use < first_assignment):
                    issues.append(CodeIssue(
                        issue_id=f"func_{i}_uninitialized_var",
                        category=ReviewCategory.FUNCTIONALITY,
                        severity=ReviewSeverity.WARNING,
                        title="Potentially uninitialized variable",
                        description=f"Variable '{var_name}' may be used before initialization",
                        line_number=i,
                        column=line.find(var_name),
                        code_snippet=line_stripped,
                        suggestion=f"Initialize '{var_name}' at declaration",
                        explanation="Uninitialized variables can contain garbage values",
                        auto_fixable=True,
                        confidence=0.7
                    ))
        
        return issues

    def _analyze_embedded_practices(self, code: str) -> List[CodeIssue]:
        """Analyze embedded systems best practices"""
        issues = []
        lines = code.split('\n')
        
        # Check for delay() usage in interrupt contexts
        in_isr = False
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Track if we're in an ISR
            if any(re.search(pattern, line_stripped) for pattern in self.embedded_patterns["interrupt_handlers"]):
                in_isr = True
            elif '}' in line_stripped and in_isr:
                in_isr = False
            
            # Check for delay in ISR
            if in_isr and any(re.search(pattern, line_stripped) for pattern in self.embedded_patterns["timing_functions"]):
                issues.append(CodeIssue(
                    issue_id=f"embed_{i}_delay_in_isr",
                    category=ReviewCategory.EMBEDDED_BEST_PRACTICES,
                    severity=ReviewSeverity.ERROR,
                    title="Delay function in interrupt handler",
                    description="Blocking delay functions should not be used in ISRs",
                    line_number=i,
                    column=line.find('delay'),
                    code_snippet=line_stripped,
                    suggestion="Use timer-based state machines or flags instead",
                    explanation="Delays in ISRs block other interrupts and affect real-time behavior",
                    auto_fixable=False,
                    confidence=0.95
                ))
            
            # Check for missing volatile on shared variables
            if re.search(r'(int|float|char|bool)\s+(\w+)\s*;', line_stripped):
                var_name = re.search(r'(int|float|char|bool)\s+(\w+)\s*;', line_stripped).group(2)
                # Check if variable is used in ISR context
                isr_usage = any(var_name in l for l in lines 
                              if any(re.search(p, l) for p in self.embedded_patterns["interrupt_handlers"]))
                if isr_usage and 'volatile' not in line_stripped:
                    issues.append(CodeIssue(
                        issue_id=f"embed_{i}_missing_volatile",
                        category=ReviewCategory.EMBEDDED_BEST_PRACTICES,
                        severity=ReviewSeverity.WARNING,
                        title="Missing volatile keyword",
                        description=f"Variable '{var_name}' used in ISR should be declared volatile",
                        line_number=i,
                        column=line.find(var_name),
                        code_snippet=line_stripped,
                        suggestion=f"Declare as 'volatile {line_stripped}'",
                        explanation="Volatile prevents compiler optimization for interrupt-shared variables",
                        auto_fixable=True,
                        confidence=0.8
                    ))
            
            # Check for direct register manipulation without comments
            if re.search(r'\b[A-Z]{2,}[0-9]*[A-Z]*\s*[|&=]', line_stripped):
                # Look for comments on same line or previous line
                has_comment = ('//' in line or '/*' in line or 
                             (i > 1 and '//' in lines[i-2]))
                if not has_comment:
                    issues.append(CodeIssue(
                        issue_id=f"embed_{i}_uncommented_register",
                        category=ReviewCategory.EMBEDDED_BEST_PRACTICES,
                        severity=ReviewSeverity.INFO,
                        title="Uncommented register operation",
                        description="Direct register manipulation should be commented",
                        line_number=i,
                        column=0,
                        code_snippet=line_stripped,
                        suggestion="Add comment explaining the register operation",
                        explanation="Register operations are hardware-specific and need documentation",
                        auto_fixable=False,
                        confidence=0.6
                    ))
        
        return issues

    def _analyze_performance(self, code: str) -> List[CodeIssue]:
        """Analyze performance-related issues"""
        issues = []
        lines = code.split('\n')
        
        # Check function length
        in_function = False
        function_start = 0
        function_name = ""
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Detect function start
            func_match = re.search(r'(void|int|float|char)\s+(\w+)\s*\([^)]*\)\s*{', line_stripped)
            if func_match:
                in_function = True
                function_start = i
                function_name = func_match.group(2)
            
            # Detect function end
            elif in_function and line_stripped == '}':
                function_length = i - function_start
                if function_length > self.performance_thresholds["max_function_lines"]:
                    issues.append(CodeIssue(
                        issue_id=f"perf_{function_start}_long_function",
                        category=ReviewCategory.PERFORMANCE,
                        severity=ReviewSeverity.WARNING,
                        title="Function too long",
                        description=f"Function '{function_name}' has {function_length} lines",
                        line_number=function_start,
                        column=0,
                        code_snippet=f"Function {function_name} ({function_length} lines)",
                        suggestion="Break into smaller functions",
                        explanation="Long functions are harder to optimize and test",
                        auto_fixable=False,
                        confidence=0.8
                    ))
                in_function = False
            
            # Check for inefficient string operations
            if re.search(r'String\s*\+|strcat|sprintf', line_stripped):
                issues.append(CodeIssue(
                    issue_id=f"perf_{i}_string_concat",
                    category=ReviewCategory.PERFORMANCE,
                    severity=ReviewSeverity.WARNING,
                    title="Inefficient string operation",
                    description="String concatenation can be memory intensive",
                    line_number=i,
                    column=0,
                    code_snippet=line_stripped,
                    suggestion="Use fixed-size buffers or snprintf for better control",
                    explanation="String operations can cause memory fragmentation",
                    auto_fixable=False,
                    confidence=0.7
                ))
            
            # Check for floating point in tight loops
            if 'for' in line_stripped or 'while' in line_stripped:
                # Look for floating point operations in next few lines
                loop_body = lines[i:i+10]
                if any(re.search(r'float|double|\.\d+|sin|cos|sqrt', l) for l in loop_body):
                    issues.append(CodeIssue(
                        issue_id=f"perf_{i}_float_in_loop",
                        category=ReviewCategory.PERFORMANCE,
                        severity=ReviewSeverity.INFO,
                        title="Floating point in loop",
                        description="Floating point operations in tight loops can impact performance",
                        line_number=i,
                        column=0,
                        code_snippet=line_stripped,
                        suggestion="Consider fixed-point arithmetic or move calculations outside loop",
                        explanation="MCUs often lack hardware FPU, making float operations slow",
                        auto_fixable=False,
                        confidence=0.6
                    ))
        
        return issues

    def _analyze_security(self, code: str) -> List[CodeIssue]:
        """Analyze security vulnerabilities"""
        issues = []
        lines = code.split('\n')
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Check for buffer overflow vulnerabilities
            for pattern in self.security_patterns["buffer_overflow"]:
                if re.search(pattern, line_stripped):
                    issues.append(CodeIssue(
                        issue_id=f"sec_{i}_buffer_overflow",
                        category=ReviewCategory.SECURITY,
                        severity=ReviewSeverity.CRITICAL,
                        title="Potential buffer overflow",
                        description="Unsafe function that can cause buffer overflow",
                        line_number=i,
                        column=line.find(re.search(pattern, line_stripped).group()),
                        code_snippet=line_stripped,
                        suggestion="Use safe alternatives like strncpy, snprintf",
                        explanation="Buffer overflows can lead to code execution vulnerabilities",
                        auto_fixable=True,
                        confidence=0.9
                    ))
            
            # Check for hardcoded secrets
            for pattern in self.security_patterns["hardcoded_secrets"]:
                if re.search(pattern, line_stripped, re.IGNORECASE):
                    issues.append(CodeIssue(
                        issue_id=f"sec_{i}_hardcoded_secret",
                        category=ReviewCategory.SECURITY,
                        severity=ReviewSeverity.ERROR,
                        title="Hardcoded secret detected",
                        description="Secrets should not be hardcoded in source code",
                        line_number=i,
                        column=0,
                        code_snippet="[REDACTED]",
                        suggestion="Use secure storage or configuration files",
                        explanation="Hardcoded secrets can be extracted from firmware",
                        auto_fixable=False,
                        confidence=0.8
                    ))
            
            # Check for weak random number generation
            for pattern in self.security_patterns["weak_random"]:
                if re.search(pattern, line_stripped):
                    issues.append(CodeIssue(
                        issue_id=f"sec_{i}_weak_random",
                        category=ReviewCategory.SECURITY,
                        severity=ReviewSeverity.WARNING,
                        title="Weak random number generation",
                        description="Standard rand() function is not cryptographically secure",
                        line_number=i,
                        column=line.find('rand'),
                        code_snippet=line_stripped,
                        suggestion="Use hardware random number generator if available",
                        explanation="Predictable random numbers can compromise security",
                        auto_fixable=False,
                        confidence=0.7
                    ))
        
        return issues

    def _analyze_memory_management(self, code: str) -> List[CodeIssue]:
        """Analyze memory management issues"""
        issues = []
        lines = code.split('\n')
        
        # Track malloc/free pairs
        mallocs = []
        frees = []
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Check for dynamic allocation in embedded systems
            if any(re.search(pattern, line_stripped) for pattern in self.embedded_patterns["memory_operations"]):
                issues.append(CodeIssue(
                    issue_id=f"mem_{i}_dynamic_allocation",
                    category=ReviewCategory.MEMORY_MANAGEMENT,
                    severity=ReviewSeverity.WARNING,
                    title="Dynamic memory allocation",
                    description="Dynamic allocation not recommended in embedded systems",
                    line_number=i,
                    column=line.find('malloc') if 'malloc' in line else line.find('free'),
                    code_snippet=line_stripped,
                    suggestion="Use static allocation or memory pools",
                    explanation="Dynamic allocation can cause fragmentation and unpredictable timing",
                    auto_fixable=False,
                    confidence=0.8
                ))
            
            # Check for large stack allocations
            array_match = re.search(r'(int|float|char)\s+\w+\[(\d+)\]', line_stripped)
            if array_match:
                size = int(array_match.group(2))
                if size > 100:  # Assuming 4-byte ints, >400 bytes
                    issues.append(CodeIssue(
                        issue_id=f"mem_{i}_large_stack_array",
                        category=ReviewCategory.MEMORY_MANAGEMENT,
                        severity=ReviewSeverity.WARNING,
                        title="Large stack allocation",
                        description=f"Large array ({size} elements) allocated on stack",
                        line_number=i,
                        column=0,
                        code_snippet=line_stripped,
                        suggestion="Consider static allocation or reduce size",
                        explanation="Large stack allocations can cause stack overflow",
                        auto_fixable=False,
                        confidence=0.7
                    ))
            
            # Check for memory leaks (malloc without free)
            if 'malloc' in line_stripped:
                var_match = re.search(r'(\w+)\s*=\s*malloc', line_stripped)
                if var_match:
                    mallocs.append((var_match.group(1), i))
            
            if 'free' in line_stripped:
                var_match = re.search(r'free\s*\(\s*(\w+)', line_stripped)
                if var_match:
                    frees.append((var_match.group(1), i))
        
        # Check for unmatched malloc/free
        for var_name, line_num in mallocs:
            if not any(var == var_name for var, _ in frees):
                issues.append(CodeIssue(
                    issue_id=f"mem_{line_num}_memory_leak",
                    category=ReviewCategory.MEMORY_MANAGEMENT,
                    severity=ReviewSeverity.ERROR,
                    title="Potential memory leak",
                    description=f"malloc for '{var_name}' without corresponding free",
                    line_number=line_num,
                    column=0,
                    code_snippet=f"malloc for {var_name}",
                    suggestion=f"Add free({var_name}) when no longer needed",
                    explanation="Memory leaks can exhaust available RAM",
                    auto_fixable=False,
                    confidence=0.8
                ))
        
        return issues

    def _analyze_timing_constraints(self, code: str) -> List[CodeIssue]:
        """Analyze timing and real-time constraints"""
        issues = []
        lines = code.split('\n')
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Check for long delays in main loop
            delay_match = re.search(r'delay\s*\(\s*(\d+)', line_stripped)
            if delay_match:
                delay_time = int(delay_match.group(1))
                if delay_time > 1000:  # > 1 second
                    issues.append(CodeIssue(
                        issue_id=f"timing_{i}_long_delay",
                        category=ReviewCategory.TIMING,
                        severity=ReviewSeverity.WARNING,
                        title="Long blocking delay",
                        description=f"Delay of {delay_time}ms blocks execution",
                        line_number=i,
                        column=line.find('delay'),
                        code_snippet=line_stripped,
                        suggestion="Use non-blocking timing or state machines",
                        explanation="Long delays prevent system from responding to other events",
                        auto_fixable=False,
                        confidence=0.8
                    ))
            
            # Check for busy-wait loops
            if re.search(r'while\s*\([^)]*digitalRead[^)]*\)', line_stripped):
                issues.append(CodeIssue(
                    issue_id=f"timing_{i}_busy_wait",
                    category=ReviewCategory.TIMING,
                    severity=ReviewSeverity.INFO,
                    title="Busy-wait loop detected",
                    description="Polling loop wastes CPU cycles",
                    line_number=i,
                    column=0,
                    code_snippet=line_stripped,
                    suggestion="Consider using interrupts or adding small delays",
                    explanation="Busy-waiting prevents power saving and blocks other tasks",
                    auto_fixable=False,
                    confidence=0.7
                ))
        
        return issues

    def _analyze_hardware_interface(self, code: str) -> List[CodeIssue]:
        """Analyze hardware interface issues"""
        issues = []
        lines = code.split('\n')
        
        for i, line in enumerate(lines, 1):
            line_stripped = line.strip()
            
            # Check for missing pin mode configuration
            if 'digitalWrite' in line_stripped or 'analogWrite' in line_stripped:
                pin_match = re.search(r'(digitalWrite|analogWrite)\s*\(\s*(\w+)', line_stripped)
                if pin_match:
                    pin_var = pin_match.group(2)
                    # Look for pinMode declaration
                    has_pin_mode = any(f"pinMode({pin_var}" in l or f"pinMode( {pin_var}" in l 
                                     for l in lines[:i])
                    if not has_pin_mode:
                        issues.append(CodeIssue(
                            issue_id=f"hw_{i}_missing_pin_mode",
                            category=ReviewCategory.HARDWARE_INTERFACE,
                            severity=ReviewSeverity.WARNING,
                            title="Missing pin mode configuration",
                            description=f"Pin {pin_var} used without pinMode configuration",
                            line_number=i,
                            column=0,
                            code_snippet=line_stripped,
                            suggestion=f"Add pinMode({pin_var}, OUTPUT) in setup()",
                            explanation="Pins should be configured before use",
                            auto_fixable=True,
                            confidence=0.8
                        ))
            
            # Check for missing pull-up resistor configuration
            if 'digitalRead' in line_stripped:
                pin_match = re.search(r'digitalRead\s*\(\s*(\w+)', line_stripped)
                if pin_match:
                    pin_var = pin_match.group(2)
                    # Check if it's configured as INPUT_PULLUP
                    has_pullup = any(f"INPUT_PULLUP" in l and pin_var in l for l in lines[:i])
                    if not has_pullup:
                        issues.append(CodeIssue(
                            issue_id=f"hw_{i}_missing_pullup",
                            category=ReviewCategory.HARDWARE_INTERFACE,
                            severity=ReviewSeverity.INFO,
                            title="Consider pull-up resistor",
                            description=f"Input pin {pin_var} might need pull-up resistor",
                            line_number=i,
                            column=0,
                            code_snippet=line_stripped,
                            suggestion=f"Use pinMode({pin_var}, INPUT_PULLUP) if needed",
                            explanation="Floating inputs can cause erratic behavior",
                            auto_fixable=False,
                            confidence=0.5
                        ))
        
        return issues

    def _calculate_complexity_score(self, code: str) -> float:
        """Calculate cyclomatic complexity"""
        decision_points = 0
        decision_keywords = ['if', 'else', 'elif', 'while', 'for', 'switch', 'case', '&&', '||', '?']
        
        for keyword in decision_keywords:
            decision_points += code.lower().count(keyword)
        
        # Base complexity of 1 + decision points
        complexity = 1 + decision_points
        
        # Normalize to 0-1 scale (max reasonable complexity of 50)
        return min(complexity / 50.0, 1.0)

    def _calculate_maintainability_index(self, code: str) -> float:
        """Calculate maintainability index"""
        lines = code.split('\n')
        non_empty_lines = [l for l in lines if l.strip()]
        
        # Factors affecting maintainability
        total_lines = len(non_empty_lines)
        comment_lines = len([l for l in lines if '//' in l or '/*' in l])
        comment_ratio = comment_lines / max(total_lines, 1)
        
        # Function count and average length
        functions = re.findall(r'(void|int|float|char)\s+\w+\s*\([^)]*\)\s*{', code)
        avg_function_length = total_lines / max(len(functions), 1)
        
        # Calculate maintainability score
        maintainability = 1.0
        maintainability *= min(comment_ratio * 5, 1.0)  # Reward comments
        maintainability *= max(0.2, 1.0 - (avg_function_length / 100))  # Penalize long functions
        
        return maintainability

    def _calculate_embedded_compliance(self, code: str) -> float:
        """Calculate embedded systems compliance score"""
        score = 1.0
        
        # Check for embedded best practices
        has_volatile = 'volatile' in code
        has_isr = any(re.search(pattern, code) for pattern in self.embedded_patterns["interrupt_handlers"])
        has_delay_in_isr = has_isr and any(re.search(pattern, code) for pattern in self.embedded_patterns["timing_functions"])
        has_dynamic_alloc = any(re.search(pattern, code) for pattern in self.embedded_patterns["memory_operations"])
        
        # Scoring
        if has_isr and not has_volatile:
            score -= 0.2  # ISR without volatile variables
        if has_delay_in_isr:
            score -= 0.3  # Delay in ISR
        if has_dynamic_alloc:
            score -= 0.2  # Dynamic allocation
        
        return max(score, 0.0)

    def _analyze_performance_metrics(self, code: str) -> List[PerformanceMetric]:
        """Analyze specific performance metrics"""
        metrics = []
        
        # Function count and complexity
        functions = re.findall(r'(void|int|float|char)\s+\w+\s*\([^)]*\)\s*{', code)
        if functions:
            metrics.append(PerformanceMetric(
                metric_name="Function Count",
                current_value=len(functions),
                recommended_value=10,
                unit="functions",
                impact="More functions can improve modularity but may increase overhead"
            ))
        
        # Code size estimation
        lines = [l for l in code.split('\n') if l.strip()]
        estimated_flash = len(lines) * 4  # Rough estimate: 4 bytes per line
        metrics.append(PerformanceMetric(
            metric_name="Estimated Flash Usage",
            current_value=estimated_flash,
            recommended_value=8192,  # 8KB for small MCUs
            unit="bytes",
            impact="Higher flash usage reduces available space for other code"
        ))
        
        # Memory usage estimation
        variables = re.findall(r'(int|float|char)\s+\w+\[(\d+)\]', code)
        total_array_size = sum(int(size) * 4 for _, size in variables)  # Assume 4 bytes per element
        metrics.append(PerformanceMetric(
            metric_name="Estimated RAM Usage (Arrays)",
            current_value=total_array_size,
            recommended_value=512,  # 512 bytes for small MCUs
            unit="bytes",
            impact="High RAM usage can cause stack overflow"
        ))
        
        return metrics

    def _generate_recommendations(self, issues: List[CodeIssue], code: str) -> List[str]:
        """Generate high-level recommendations"""
        recommendations = []
        
        # Count issues by category
        category_counts = {}
        for issue in issues:
            category_counts[issue.category] = category_counts.get(issue.category, 0) + 1
        
        # Generate category-specific recommendations
        if category_counts.get(ReviewCategory.PERFORMANCE, 0) > 2:
            recommendations.append("Consider performance optimization - multiple performance issues detected")
        
        if category_counts.get(ReviewCategory.SECURITY, 0) > 0:
            recommendations.append("Address security vulnerabilities before deployment")
        
        if category_counts.get(ReviewCategory.EMBEDDED_BEST_PRACTICES, 0) > 1:
            recommendations.append("Review embedded systems best practices and design patterns")
        
        if 'delay(' in code and 'interrupt' in code.lower():
            recommendations.append("Consider using timer-based state machines instead of blocking delays")
        
        if 'malloc' in code:
            recommendations.append("Avoid dynamic memory allocation in embedded systems")
        
        return recommendations

    def _generate_auto_fixes(self, issues: List[CodeIssue]) -> List[str]:
        """Generate automatic fixes for fixable issues"""
        auto_fixes = []
        
        for issue in issues:
            if issue.auto_fixable:
                auto_fixes.append(f"Line {issue.line_number}: {issue.suggestion}")
        
        return auto_fixes

    def _calculate_overall_score(self, report: CodeReviewReport) -> float:
        """Calculate overall code quality score"""
        base_score = 1.0
        
        # Deduct points for issues based on severity
        for issue in report.issues:
            if issue.severity == ReviewSeverity.CRITICAL:
                base_score -= 0.3
            elif issue.severity == ReviewSeverity.ERROR:
                base_score -= 0.2
            elif issue.severity == ReviewSeverity.WARNING:
                base_score -= 0.1
            elif issue.severity == ReviewSeverity.INFO:
                base_score -= 0.05
        
        # Factor in other metrics
        base_score *= (1.0 - report.complexity_score * 0.2)  # Penalize high complexity
        base_score *= report.maintainability_index * 0.3 + 0.7  # Reward maintainability
        base_score *= report.embedded_compliance_score * 0.2 + 0.8  # Reward compliance
        
        return max(base_score, 0.0)

    def _find_function_end(self, lines: List[str], start: int) -> int:
        """Find the end of a function starting at given line"""
        brace_count = 0
        for i in range(start, len(lines)):
            brace_count += lines[i].count('{') - lines[i].count('}')
            if brace_count == 0 and i > start:
                return i
        return len(lines)

    def _save_review_report(self, report: CodeReviewReport, file_hash: str):
        """Save review report to database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        review_id = f"review_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{file_hash[:8]}"
        
        cursor.execute('''
            INSERT INTO code_reviews (review_id, file_path, file_hash, review_data)
            VALUES (?, ?, ?, ?)
        ''', (review_id, report.file_path, file_hash, json.dumps(asdict(report), default=str)))
        
        # Save metrics
        for metric in report.performance_metrics:
            cursor.execute('''
                INSERT INTO review_metrics (review_id, metric_name, metric_value)
                VALUES (?, ?, ?)
            ''', (review_id, metric.metric_name, metric.current_value))
        
        conn.commit()
        conn.close()

    def generate_review_summary(self, report: CodeReviewReport) -> str:
        """Generate human-readable review summary"""
        summary = f"""
📊 Code Review Summary for {report.file_path}
{'='*60}

🎯 Overall Score: {report.overall_score:.2f}/1.0
📈 Complexity Score: {report.complexity_score:.2f}/1.0
🔧 Maintainability Index: {report.maintainability_index:.2f}/1.0
🔌 Embedded Compliance: {report.embedded_compliance_score:.2f}/1.0

🔍 Issues Found: {len(report.issues)}
"""
        
        # Group issues by severity
        critical = [i for i in report.issues if i.severity == ReviewSeverity.CRITICAL]
        errors = [i for i in report.issues if i.severity == ReviewSeverity.ERROR]
        warnings = [i for i in report.issues if i.severity == ReviewSeverity.WARNING]
        info = [i for i in report.issues if i.severity == ReviewSeverity.INFO]
        
        if critical:
            summary += f"🚨 Critical: {len(critical)}\n"
        if errors:
            summary += f"❌ Errors: {len(errors)}\n"
        if warnings:
            summary += f"⚠️  Warnings: {len(warnings)}\n"
        if info:
            summary += f"ℹ️  Info: {len(info)}\n"
        
        # Add top issues
        summary += "\n🔝 Top Issues:\n"
        top_issues = sorted(report.issues, 
                          key=lambda x: {'critical': 4, 'error': 3, 'warning': 2, 'info': 1}[x.severity.value],
                          reverse=True)[:5]
        
        for i, issue in enumerate(top_issues, 1):
            summary += f"{i}. Line {issue.line_number}: {issue.title}\n"
        
        # Add recommendations
        if report.recommendations:
            summary += "\n💡 Recommendations:\n"
            for rec in report.recommendations:
                summary += f"• {rec}\n"
        
        return summary

# Example usage
if __name__ == "__main__":
    # Initialize code reviewer
    reviewer = IntelligentCodeReviewer()
    
    # Example embedded C code for review
    sample_code = """
#include <Arduino.h>

int sensor_value;
float temperature;

void setup() {
    Serial.begin(9600);
    // Missing pinMode configuration
    digitalWrite(13, HIGH);
}

ISR(TIMER1_COMPA_vect) {
    sensor_value = analogRead(A0);
    delay(100);  // Bad: delay in ISR
    Serial.println(sensor_value);  // Bad: Serial in ISR
}

void calculate_average() {
    float values[1000];  // Large stack allocation
    for(int i = 0; i < 1000; i++) {
        values[i] = sin(i * 3.14159 / 180);  // Float in loop
    }
    
    char* buffer = malloc(256);  // Dynamic allocation
    sprintf(buffer, "Average calculated");  // Unsafe function
    Serial.println(buffer);
    // Missing free(buffer)
}

void loop() {
    while(digitalRead(2)) {  // Busy wait
        // Do nothing
    }
    
    delay(5000);  // Long delay
    calculate_average();
}
"""
    
    # Perform code review
    report = reviewer.review_code_file("example.ino", sample_code)
    
    # Print summary
    print(reviewer.generate_review_summary(report))
    
    # Print detailed issues
    print("\n📋 Detailed Issues:")
    for issue in report.issues:
        print(f"\n{issue.severity.value.upper()}: {issue.title}")
        print(f"  Line {issue.line_number}: {issue.description}")
        print(f"  Suggestion: {issue.suggestion}")
        print(f"  Confidence: {issue.confidence:.1%}")
    
    print(f"\n✅ Code review completed! Found {len(report.issues)} issues.")