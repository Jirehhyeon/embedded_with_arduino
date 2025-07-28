#!/usr/bin/env python3
"""
AI-Powered Learning Assistant for Embedded Projects Platform
Description: Intelligent tutoring system with personalized learning paths
Version: 1.0
Author: Embedded Projects Platform Team
"""

import json
import sqlite3
import numpy as np
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from enum import Enum
import re
import ast

class LearningStyle(Enum):
    VISUAL = "visual"
    AUDITORY = "auditory"
    KINESTHETIC = "kinesthetic"
    READING = "reading"

class DifficultyLevel(Enum):
    BEGINNER = 1
    INTERMEDIATE = 2
    ADVANCED = 3
    EXPERT = 4

@dataclass
class StudentProfile:
    student_id: str
    name: str
    learning_style: LearningStyle
    current_level: DifficultyLevel
    completed_projects: List[int]
    skill_scores: Dict[str, float]  # skill -> score (0.0-1.0)
    learning_preferences: Dict[str, any]
    last_activity: datetime
    total_study_time: int  # minutes
    strengths: List[str]
    weaknesses: List[str]

@dataclass
class LearningPath:
    path_id: str
    student_id: str
    target_skills: List[str]
    recommended_projects: List[int]
    estimated_duration: int  # hours
    current_step: int
    completion_percentage: float
    adaptive_adjustments: List[str]

@dataclass
class CodeAnalysis:
    file_path: str
    complexity_score: float
    quality_score: float
    issues: List[Dict[str, any]]
    suggestions: List[str]
    skill_demonstrations: List[str]
    improvement_areas: List[str]

class AILearningAssistant:
    def __init__(self, db_path: str = "learning_assistant.db"):
        self.db_path = db_path
        self.init_database()
        
        # Knowledge base for embedded C concepts
        self.concepts = {
            "gpio": {
                "difficulty": 1,
                "prerequisites": [],
                "description": "General Purpose Input/Output control",
                "code_patterns": ["DDRB", "PORTB", "PINB", "digitalWrite", "digitalRead"],
                "common_mistakes": ["forgetting DDR configuration", "pin numbering confusion"]
            },
            "timers": {
                "difficulty": 2,
                "prerequisites": ["gpio"],
                "description": "Timer/Counter peripherals and PWM",
                "code_patterns": ["TCCR", "OCR", "analogWrite", "timer interrupt"],
                "common_mistakes": ["prescaler calculation", "timer overflow handling"]
            },
            "interrupts": {
                "difficulty": 3,
                "prerequisites": ["gpio", "timers"],
                "description": "Interrupt-driven programming",
                "code_patterns": ["ISR", "attachInterrupt", "sei()", "cli()"],
                "common_mistakes": ["long ISR routines", "non-atomic operations"]
            },
            "adc": {
                "difficulty": 2,
                "prerequisites": ["gpio"],
                "description": "Analog-to-Digital Conversion",
                "code_patterns": ["ADMUX", "ADCSRA", "analogRead", "ADC_vect"],
                "common_mistakes": ["reference voltage selection", "conversion timing"]
            },
            "uart": {
                "difficulty": 2,
                "prerequisites": ["gpio"],
                "description": "Universal Asynchronous Receiver-Transmitter",
                "code_patterns": ["Serial.begin", "UDR", "UBRR", "UART_vect"],
                "common_mistakes": ["baud rate calculation", "buffer overflow"]
            },
            "i2c": {
                "difficulty": 3,
                "prerequisites": ["uart"],
                "description": "Inter-Integrated Circuit communication",
                "code_patterns": ["Wire.begin", "TWI", "TWCR", "TWDR"],
                "common_mistakes": ["address conflicts", "clock stretching"]
            },
            "spi": {
                "difficulty": 3,
                "prerequisites": ["gpio"],
                "description": "Serial Peripheral Interface",
                "code_patterns": ["SPCR", "SPDR", "SPI.begin", "SPI.transfer"],
                "common_mistakes": ["clock polarity", "slave select timing"]
            },
            "rtos": {
                "difficulty": 4,
                "prerequisites": ["interrupts", "timers"],
                "description": "Real-Time Operating System concepts",
                "code_patterns": ["xTaskCreate", "vTaskDelay", "xQueueSend"],
                "common_mistakes": ["stack overflow", "priority inversion"]
            }
        }
        
        # Project difficulty and skill mappings
        self.project_skills = {
            1: ["gpio"],
            2: ["gpio", "timers"],
            3: ["gpio", "adc"],
            4: ["gpio", "uart"],
            5: ["gpio", "interrupts"],
            6: ["i2c", "uart"],
            7: ["gpio", "timers", "interrupts"],
            8: ["spi", "uart"],
            9: ["gpio", "timers", "adc", "uart"],
            10: ["rtos", "interrupts", "timers"]
        }

    def init_database(self):
        """Initialize SQLite database for learning analytics"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Student profiles table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS student_profiles (
                student_id TEXT PRIMARY KEY,
                profile_data TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Learning sessions table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS learning_sessions (
                session_id INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id TEXT,
                project_id INTEGER,
                start_time TIMESTAMP,
                end_time TIMESTAMP,
                code_submissions INTEGER DEFAULT 0,
                errors_encountered INTEGER DEFAULT 0,
                hints_requested INTEGER DEFAULT 0,
                completion_status TEXT,
                FOREIGN KEY (student_id) REFERENCES student_profiles (student_id)
            )
        ''')
        
        # Code analysis results table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS code_analysis (
                analysis_id INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id TEXT,
                project_id INTEGER,
                file_path TEXT,
                analysis_data TEXT,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (student_id) REFERENCES student_profiles (student_id)
            )
        ''')
        
        # Learning paths table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS learning_paths (
                path_id TEXT PRIMARY KEY,
                student_id TEXT,
                path_data TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (student_id) REFERENCES student_profiles (student_id)
            )
        ''')
        
        conn.commit()
        conn.close()

    def create_student_profile(self, student_id: str, name: str, 
                             assessment_results: Dict[str, any]) -> StudentProfile:
        """Create a new student profile based on initial assessment"""
        
        # Determine learning style from assessment
        learning_style = self._determine_learning_style(assessment_results)
        
        # Determine current level
        current_level = self._assess_skill_level(assessment_results)
        
        # Initialize skill scores
        skill_scores = {}
        for skill in self.concepts.keys():
            skill_scores[skill] = assessment_results.get(f"skill_{skill}", 0.0)
        
        # Create profile
        profile = StudentProfile(
            student_id=student_id,
            name=name,
            learning_style=learning_style,
            current_level=current_level,
            completed_projects=[],
            skill_scores=skill_scores,
            learning_preferences=assessment_results.get("preferences", {}),
            last_activity=datetime.now(),
            total_study_time=0,
            strengths=self._identify_strengths(skill_scores),
            weaknesses=self._identify_weaknesses(skill_scores)
        )
        
        # Save to database
        self._save_student_profile(profile)
        
        return profile

    def generate_personalized_learning_path(self, student_id: str, 
                                          target_certification: str = "AED") -> LearningPath:
        """Generate adaptive learning path based on student profile"""
        
        profile = self._load_student_profile(student_id)
        if not profile:
            raise ValueError(f"Student profile not found: {student_id}")
        
        # Determine target skills based on certification
        target_skills = self._get_certification_skills(target_certification)
        
        # Filter skills student needs to improve
        skills_to_learn = []
        for skill in target_skills:
            if profile.skill_scores.get(skill, 0.0) < 0.8:  # 80% proficiency threshold
                skills_to_learn.append(skill)
        
        # Generate recommended project sequence
        recommended_projects = self._recommend_project_sequence(
            profile, skills_to_learn
        )
        
        # Estimate duration based on student's learning pattern
        estimated_duration = self._estimate_learning_duration(
            profile, recommended_projects
        )
        
        # Create learning path
        path = LearningPath(
            path_id=f"path_{student_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}",
            student_id=student_id,
            target_skills=skills_to_learn,
            recommended_projects=recommended_projects,
            estimated_duration=estimated_duration,
            current_step=0,
            completion_percentage=0.0,
            adaptive_adjustments=[]
        )
        
        # Save learning path
        self._save_learning_path(path)
        
        return path

    def analyze_code_submission(self, student_id: str, project_id: int, 
                              code_content: str, file_path: str) -> CodeAnalysis:
        """Analyze student's code submission and provide feedback"""
        
        # Static code analysis
        complexity_score = self._calculate_complexity(code_content)
        quality_score = self._assess_code_quality(code_content)
        
        # Identify issues and suggestions
        issues = self._find_code_issues(code_content)
        suggestions = self._generate_suggestions(code_content, project_id)
        
        # Identify demonstrated skills
        skill_demonstrations = self._identify_demonstrated_skills(code_content)
        
        # Identify improvement areas
        improvement_areas = self._identify_improvement_areas(
            code_content, project_id
        )
        
        # Create analysis result
        analysis = CodeAnalysis(
            file_path=file_path,
            complexity_score=complexity_score,
            quality_score=quality_score,
            issues=issues,
            suggestions=suggestions,
            skill_demonstrations=skill_demonstrations,
            improvement_areas=improvement_areas
        )
        
        # Save analysis to database
        self._save_code_analysis(student_id, project_id, analysis)
        
        # Update student profile based on analysis
        self._update_student_skills(student_id, analysis)
        
        return analysis

    def provide_contextual_hint(self, student_id: str, project_id: int, 
                              current_code: str, error_message: str = "") -> str:
        """Provide intelligent hints based on context"""
        
        profile = self._load_student_profile(student_id)
        project_skills = self.project_skills.get(project_id, [])
        
        # Analyze current code to understand progress
        progress = self._analyze_code_progress(current_code, project_id)
        
        # Generate contextual hint
        if error_message:
            hint = self._generate_error_hint(error_message, project_skills)
        else:
            hint = self._generate_progress_hint(progress, project_skills, profile)
        
        # Adapt hint to learning style
        adapted_hint = self._adapt_hint_to_learning_style(hint, profile.learning_style)
        
        return adapted_hint

    def generate_practice_problems(self, student_id: str, skill: str, 
                                 difficulty: int = None) -> List[Dict[str, any]]:
        """Generate personalized practice problems"""
        
        profile = self._load_student_profile(student_id)
        
        if difficulty is None:
            difficulty = int(profile.current_level.value)
        
        problems = []
        
        if skill == "gpio":
            problems.extend(self._generate_gpio_problems(difficulty))
        elif skill == "timers":
            problems.extend(self._generate_timer_problems(difficulty))
        elif skill == "interrupts":
            problems.extend(self._generate_interrupt_problems(difficulty))
        elif skill == "adc":
            problems.extend(self._generate_adc_problems(difficulty))
        elif skill == "uart":
            problems.extend(self._generate_uart_problems(difficulty))
        
        # Adapt problems to learning style
        adapted_problems = []
        for problem in problems:
            adapted_problem = self._adapt_problem_to_learning_style(
                problem, profile.learning_style
            )
            adapted_problems.append(adapted_problem)
        
        return adapted_problems

    def track_learning_session(self, student_id: str, project_id: int):
        """Track and analyze learning session"""
        
        # This would be called at the start of a learning session
        # and would track various metrics throughout the session
        
        session_data = {
            "student_id": student_id,
            "project_id": project_id,
            "start_time": datetime.now(),
            "code_submissions": 0,
            "errors_encountered": 0,
            "hints_requested": 0
        }
        
        return session_data

    def _determine_learning_style(self, assessment: Dict[str, any]) -> LearningStyle:
        """Determine learning style from assessment results"""
        
        style_scores = {
            LearningStyle.VISUAL: assessment.get("visual_score", 0),
            LearningStyle.AUDITORY: assessment.get("auditory_score", 0),
            LearningStyle.KINESTHETIC: assessment.get("kinesthetic_score", 0),
            LearningStyle.READING: assessment.get("reading_score", 0)
        }
        
        return max(style_scores.items(), key=lambda x: x[1])[0]

    def _assess_skill_level(self, assessment: Dict[str, any]) -> DifficultyLevel:
        """Assess overall skill level from assessment"""
        
        total_score = sum(assessment.get(f"skill_{skill}", 0) 
                         for skill in self.concepts.keys())
        avg_score = total_score / len(self.concepts)
        
        if avg_score < 0.3:
            return DifficultyLevel.BEGINNER
        elif avg_score < 0.6:
            return DifficultyLevel.INTERMEDIATE
        elif avg_score < 0.8:
            return DifficultyLevel.ADVANCED
        else:
            return DifficultyLevel.EXPERT

    def _identify_strengths(self, skill_scores: Dict[str, float]) -> List[str]:
        """Identify student's strengths"""
        return [skill for skill, score in skill_scores.items() if score > 0.7]

    def _identify_weaknesses(self, skill_scores: Dict[str, float]) -> List[str]:
        """Identify student's weaknesses"""
        return [skill for skill, score in skill_scores.items() if score < 0.5]

    def _get_certification_skills(self, certification: str) -> List[str]:
        """Get required skills for certification level"""
        
        skill_maps = {
            "AED": ["gpio", "timers", "adc", "uart"],
            "ESD": ["gpio", "timers", "adc", "uart", "interrupts", "i2c"],
            "SEE": ["gpio", "timers", "adc", "uart", "interrupts", "i2c", "spi"],
            "ESA": list(self.concepts.keys())  # All skills
        }
        
        return skill_maps.get(certification, skill_maps["AED"])

    def _recommend_project_sequence(self, profile: StudentProfile, 
                                  skills_to_learn: List[str]) -> List[int]:
        """Recommend optimal project sequence"""
        
        # Find projects that teach the required skills
        relevant_projects = []
        for project_id, project_skills in self.project_skills.items():
            if any(skill in skills_to_learn for skill in project_skills):
                relevant_projects.append(project_id)
        
        # Sort by difficulty and prerequisite order
        def project_difficulty(project_id):
            project_skills = self.project_skills[project_id]
            return max(self.concepts[skill]["difficulty"] for skill in project_skills)
        
        relevant_projects.sort(key=project_difficulty)
        
        # Remove already completed projects
        recommended = [p for p in relevant_projects 
                      if p not in profile.completed_projects]
        
        return recommended

    def _estimate_learning_duration(self, profile: StudentProfile, 
                                  projects: List[int]) -> int:
        """Estimate learning duration in hours"""
        
        base_hours_per_project = {
            DifficultyLevel.BEGINNER: 8,
            DifficultyLevel.INTERMEDIATE: 6,
            DifficultyLevel.ADVANCED: 4,
            DifficultyLevel.EXPERT: 3
        }
        
        base_hours = base_hours_per_project[profile.current_level]
        total_hours = len(projects) * base_hours
        
        # Adjust based on learning style and preferences
        if profile.learning_style == LearningStyle.KINESTHETIC:
            total_hours *= 1.2  # Hands-on learners might need more time
        elif profile.learning_style == LearningStyle.VISUAL:
            total_hours *= 0.9  # Visual learners might be faster with diagrams
        
        return int(total_hours)

    def _calculate_complexity(self, code: str) -> float:
        """Calculate cyclomatic complexity of code"""
        
        # Count decision points
        decision_keywords = ['if', 'else', 'elif', 'while', 'for', 'switch', 'case']
        complexity = 1  # Base complexity
        
        for keyword in decision_keywords:
            complexity += code.lower().count(keyword)
        
        # Normalize to 0-1 scale (assuming max reasonable complexity of 20)
        return min(complexity / 20.0, 1.0)

    def _assess_code_quality(self, code: str) -> float:
        """Assess overall code quality"""
        
        quality_score = 1.0
        
        # Check for comments
        comment_ratio = (code.count('//') + code.count('/*')) / max(code.count('\n'), 1)
        if comment_ratio < 0.1:
            quality_score -= 0.2
        
        # Check for magic numbers
        magic_numbers = re.findall(r'\b\d{2,}\b', code)
        if len(magic_numbers) > 3:
            quality_score -= 0.1
        
        # Check for function length
        functions = re.findall(r'(?:void|int|float|char)\s+\w+\s*\([^)]*\)\s*{[^}]*}', code, re.DOTALL)
        for func in functions:
            if func.count('\n') > 50:  # Very long function
                quality_score -= 0.1
        
        # Check for variable naming
        variables = re.findall(r'\b(?:int|float|char|bool)\s+(\w+)', code)
        short_names = [v for v in variables if len(v) < 3 and v not in ['i', 'j', 'k']]
        if len(short_names) > len(variables) * 0.3:
            quality_score -= 0.2
        
        return max(quality_score, 0.0)

    def _find_code_issues(self, code: str) -> List[Dict[str, any]]:
        """Find potential issues in code"""
        
        issues = []
        
        # Check for common embedded C issues
        if 'delay(' in code and 'interrupt' in code.lower():
            issues.append({
                "type": "warning",
                "message": "Using delay() in interrupt context can cause timing issues",
                "line": code.split('\n').index(next(line for line in code.split('\n') if 'delay(' in line)) + 1,
                "severity": "medium"
            })
        
        if code.count('malloc') > 0:
            issues.append({
                "type": "warning", 
                "message": "Dynamic memory allocation not recommended in embedded systems",
                "severity": "high"
            })
        
        # Check for missing volatile keyword
        if 'ISR(' in code and 'volatile' not in code:
            issues.append({
                "type": "warning",
                "message": "Consider using 'volatile' for variables accessed in ISR",
                "severity": "medium"
            })
        
        return issues

    def _generate_suggestions(self, code: str, project_id: int) -> List[str]:
        """Generate code improvement suggestions"""
        
        suggestions = []
        project_skills = self.project_skills.get(project_id, [])
        
        # Skill-specific suggestions
        if 'gpio' in project_skills:
            if 'DDR' not in code and 'pinMode' not in code:
                suggestions.append("Remember to configure pin direction using DDR registers or pinMode()")
        
        if 'timers' in project_skills:
            if 'TCCR' not in code and 'Timer' not in code:
                suggestions.append("Consider using timer registers (TCCR) for precise timing control")
        
        if 'interrupts' in project_skills:
            if 'ISR(' in code and len(code.split('\n')) > 10:
                suggestions.append("Keep Interrupt Service Routines short and fast")
        
        # General suggestions
        if code.count('\n') > 100 and code.count('//') < 5:
            suggestions.append("Add more comments to explain your code logic")
        
        return suggestions

    def _identify_demonstrated_skills(self, code: str) -> List[str]:
        """Identify which skills the code demonstrates"""
        
        demonstrated = []
        
        for skill, info in self.concepts.items():
            patterns = info["code_patterns"]
            if any(pattern.lower() in code.lower() for pattern in patterns):
                demonstrated.append(skill)
        
        return demonstrated

    def _identify_improvement_areas(self, code: str, project_id: int) -> List[str]:
        """Identify areas for improvement"""
        
        improvements = []
        project_skills = self.project_skills.get(project_id, [])
        
        for skill in project_skills:
            if skill not in self._identify_demonstrated_skills(code):
                improvements.append(f"Implement {skill} functionality")
        
        return improvements

    def _generate_gpio_problems(self, difficulty: int) -> List[Dict[str, any]]:
        """Generate GPIO-specific practice problems"""
        
        problems = []
        
        if difficulty == 1:
            problems.append({
                "title": "LED Blink Pattern",
                "description": "Create a program that blinks an LED in a specific pattern: 2 short blinks, 1 long blink, repeat",
                "hints": ["Use delay() for timing", "Consider using loops for patterns"],
                "expected_concepts": ["gpio", "timing", "loops"]
            })
        
        elif difficulty >= 2:
            problems.append({
                "title": "Button Debouncing",
                "description": "Implement proper button debouncing without using delay()",
                "hints": ["Use timer-based debouncing", "Track button state changes"],
                "expected_concepts": ["gpio", "timers", "state machines"]
            })
        
        return problems

    def _generate_timer_problems(self, difficulty: int) -> List[Dict[str, any]]:
        """Generate timer-specific practice problems"""
        
        problems = []
        
        if difficulty == 1:
            problems.append({
                "title": "PWM Motor Control",
                "description": "Control motor speed using PWM with 1kHz frequency",
                "hints": ["Calculate timer prescaler", "Use compare match for PWM"],
                "expected_concepts": ["timers", "pwm", "calculations"]
            })
        
        return problems

    def _generate_interrupt_problems(self, difficulty: int) -> List[Dict[str, any]]:
        """Generate interrupt-specific practice problems"""
        
        problems = []
        
        if difficulty >= 2:
            problems.append({
                "title": "Real-time Counter",
                "description": "Create a real-time seconds counter using timer interrupts",
                "hints": ["Use Timer1 with 1-second overflow", "Keep ISR minimal"],
                "expected_concepts": ["interrupts", "timers", "real-time"]
            })
        
        return problems

    def _generate_adc_problems(self, difficulty: int) -> List[Dict[str, any]]:
        """Generate ADC-specific practice problems"""
        
        problems = []
        
        if difficulty == 1:
            problems.append({
                "title": "Temperature Monitor",
                "description": "Read temperature sensor and display on serial monitor",
                "hints": ["Configure ADC reference", "Convert ADC values to temperature"],
                "expected_concepts": ["adc", "uart", "sensor_interfacing"]
            })
        
        return problems

    def _generate_uart_problems(self, difficulty: int) -> List[Dict[str, any]]:
        """Generate UART-specific practice problems"""
        
        problems = []
        
        if difficulty == 1:
            problems.append({
                "title": "Command Parser",
                "description": "Create a simple command parser that responds to text commands",
                "hints": ["Use String comparison", "Implement command lookup table"],
                "expected_concepts": ["uart", "string_processing", "state_machines"]
            })
        
        return problems

    def _adapt_hint_to_learning_style(self, hint: str, style: LearningStyle) -> str:
        """Adapt hint presentation to learning style"""
        
        if style == LearningStyle.VISUAL:
            return f"💡 {hint}\n📊 Consider drawing a flowchart of your logic"
        elif style == LearningStyle.AUDITORY:
            return f"🔊 {hint}\n🗣️ Try explaining your approach out loud"
        elif style == LearningStyle.KINESTHETIC:
            return f"🔧 {hint}\n⚡ Test this on actual hardware if possible"
        elif style == LearningStyle.READING:
            return f"📚 {hint}\n📖 Review the documentation for additional details"
        
        return hint

    def _adapt_problem_to_learning_style(self, problem: Dict[str, any], 
                                       style: LearningStyle) -> Dict[str, any]:
        """Adapt problem presentation to learning style"""
        
        adapted = problem.copy()
        
        if style == LearningStyle.VISUAL:
            adapted["additional_resources"] = ["Circuit diagram", "Timing diagram", "Flowchart"]
        elif style == LearningStyle.AUDITORY:
            adapted["additional_resources"] = ["Video explanation", "Audio walkthrough"]
        elif style == LearningStyle.KINESTHETIC:
            adapted["additional_resources"] = ["Hardware setup guide", "Hands-on experiments"]
        elif style == LearningStyle.READING:
            adapted["additional_resources"] = ["Detailed documentation", "Code examples", "Theory notes"]
        
        return adapted

    def _save_student_profile(self, profile: StudentProfile):
        """Save student profile to database"""
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO student_profiles (student_id, profile_data, updated_at)
            VALUES (?, ?, CURRENT_TIMESTAMP)
        ''', (profile.student_id, json.dumps(asdict(profile), default=str)))
        
        conn.commit()
        conn.close()

    def _load_student_profile(self, student_id: str) -> Optional[StudentProfile]:
        """Load student profile from database"""
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT profile_data FROM student_profiles WHERE student_id = ?
        ''', (student_id,))
        
        result = cursor.fetchone()
        conn.close()
        
        if result:
            data = json.loads(result[0])
            # Convert back to enum types
            data['learning_style'] = LearningStyle(data['learning_style'])
            data['current_level'] = DifficultyLevel(data['current_level'])
            data['last_activity'] = datetime.fromisoformat(data['last_activity'])
            return StudentProfile(**data)
        
        return None

    def _save_learning_path(self, path: LearningPath):
        """Save learning path to database"""
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO learning_paths (path_id, student_id, path_data, updated_at)
            VALUES (?, ?, ?, CURRENT_TIMESTAMP)
        ''', (path.path_id, path.student_id, json.dumps(asdict(path))))
        
        conn.commit()
        conn.close()

    def _save_code_analysis(self, student_id: str, project_id: int, analysis: CodeAnalysis):
        """Save code analysis to database"""
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO code_analysis (student_id, project_id, file_path, analysis_data)
            VALUES (?, ?, ?, ?)
        ''', (student_id, project_id, analysis.file_path, json.dumps(asdict(analysis))))
        
        conn.commit()
        conn.close()

    def _update_student_skills(self, student_id: str, analysis: CodeAnalysis):
        """Update student skill scores based on code analysis"""
        
        profile = self._load_student_profile(student_id)
        if not profile:
            return
        
        # Update skills based on demonstrated abilities
        for skill in analysis.skill_demonstrations:
            current_score = profile.skill_scores.get(skill, 0.0)
            improvement = min(0.1, analysis.quality_score * 0.1)
            profile.skill_scores[skill] = min(1.0, current_score + improvement)
        
        # Update activity timestamp
        profile.last_activity = datetime.now()
        
        # Recalculate strengths and weaknesses
        profile.strengths = self._identify_strengths(profile.skill_scores)
        profile.weaknesses = self._identify_weaknesses(profile.skill_scores)
        
        # Save updated profile
        self._save_student_profile(profile)

# Example usage and testing
if __name__ == "__main__":
    # Initialize AI assistant
    assistant = AILearningAssistant()
    
    # Example assessment results
    assessment = {
        "visual_score": 8,
        "auditory_score": 6,
        "kinesthetic_score": 9,
        "reading_score": 7,
        "skill_gpio": 0.6,
        "skill_timers": 0.3,
        "skill_interrupts": 0.1,
        "skill_adc": 0.4,
        "skill_uart": 0.2,
        "preferences": {
            "study_time": "evening",
            "session_length": 60
        }
    }
    
    # Create student profile
    profile = assistant.create_student_profile("student_001", "John Doe", assessment)
    print(f"Created profile for {profile.name}")
    print(f"Learning style: {profile.learning_style.value}")
    print(f"Current level: {profile.current_level.value}")
    print(f"Strengths: {profile.strengths}")
    print(f"Weaknesses: {profile.weaknesses}")
    
    # Generate learning path
    learning_path = assistant.generate_personalized_learning_path("student_001", "ESD")
    print(f"\nLearning path created: {learning_path.path_id}")
    print(f"Target skills: {learning_path.target_skills}")
    print(f"Recommended projects: {learning_path.recommended_projects}")
    print(f"Estimated duration: {learning_path.estimated_duration} hours")
    
    # Example code analysis
    sample_code = """
    #include <Arduino.h>
    
    const int LED_PIN = 13;
    const int BUTTON_PIN = 2;
    
    void setup() {
      pinMode(LED_PIN, OUTPUT);
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      Serial.begin(9600);
    }
    
    void loop() {
      if (digitalRead(BUTTON_PIN) == LOW) {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Button pressed");
        delay(100);  // Simple debouncing
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    }
    """
    
    analysis = assistant.analyze_code_submission("student_001", 1, sample_code, "button_led.ino")
    print(f"\nCode analysis results:")
    print(f"Quality score: {analysis.quality_score:.2f}")
    print(f"Complexity score: {analysis.complexity_score:.2f}")
    print(f"Demonstrated skills: {analysis.skill_demonstrations}")
    print(f"Suggestions: {analysis.suggestions}")
    
    # Generate practice problems
    problems = assistant.generate_practice_problems("student_001", "gpio", 1)
    print(f"\nGenerated {len(problems)} practice problems for GPIO:")
    for i, problem in enumerate(problems, 1):
        print(f"{i}. {problem['title']}: {problem['description']}")
    
    print("\n✅ AI Learning Assistant demonstration completed!")