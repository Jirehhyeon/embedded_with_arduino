#!/usr/bin/env python3
"""
Adaptive Curriculum Engine for Embedded Projects Platform
Description: AI-driven personalized learning path optimization
Version: 1.0
Author: Embedded Projects Platform Team
"""

import json
import sqlite3
import numpy as np
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple, Set
from dataclasses import dataclass, asdict
from enum import Enum
import math
import random

class SkillCategory(Enum):
    FUNDAMENTALS = "fundamentals"
    HARDWARE_INTERFACE = "hardware_interface"
    COMMUNICATION = "communication"
    REAL_TIME = "real_time"
    OPTIMIZATION = "optimization"
    SYSTEMS_DESIGN = "systems_design"

class LearningObjective(Enum):
    UNDERSTANDING = "understanding"
    APPLICATION = "application"
    ANALYSIS = "analysis"
    SYNTHESIS = "synthesis"
    EVALUATION = "evaluation"

@dataclass
class SkillNode:
    skill_id: str
    name: str
    category: SkillCategory
    difficulty_level: int  # 1-5
    prerequisites: List[str]
    learning_objectives: List[LearningObjective]
    estimated_hours: int
    projects: List[int]
    concepts: List[str]
    assessment_criteria: Dict[str, float]
    industry_relevance: float  # 0.0-1.0

@dataclass
class LearningProgress:
    student_id: str
    skill_id: str
    mastery_level: float  # 0.0-1.0
    confidence_score: float  # 0.0-1.0
    time_spent: int  # minutes
    attempts: int
    last_practice: datetime
    learning_velocity: float  # mastery gain per hour
    retention_rate: float  # 0.0-1.0
    difficulty_preference: float  # -1.0 to 1.0 (easy to hard)

@dataclass
class AdaptiveRecommendation:
    recommendation_id: str
    student_id: str
    skill_id: str
    project_id: int
    difficulty_adjustment: float  # -0.5 to 0.5
    estimated_duration: int  # minutes
    rationale: str
    confidence: float  # 0.0-1.0
    priority: float  # 0.0-1.0
    adaptive_features: List[str]

@dataclass
class CurriculumPath:
    path_id: str
    student_id: str
    target_certification: str
    current_phase: str
    skills_sequence: List[str]
    milestones: List[Dict[str, any]]
    completion_percentage: float
    estimated_completion: datetime
    adaptive_adjustments: List[str]
    learning_style_adaptations: List[str]

class AdaptiveCurriculumEngine:
    def __init__(self, db_path: str = "adaptive_curriculum.db"):
        self.db_path = db_path
        self.init_database()
        self.init_skill_graph()
        
        # Learning analytics parameters
        self.forgetting_curve_factor = 0.8  # Ebbinghaus forgetting curve
        self.mastery_threshold = 0.85
        self.confidence_threshold = 0.75
        self.learning_velocity_window = 10  # sessions
        
        # Adaptation parameters
        self.difficulty_adaptation_rate = 0.1
        self.recommendation_diversity = 0.3
        self.path_optimization_frequency = 5  # sessions

    def init_database(self):
        """Initialize database for adaptive curriculum"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Learning progress table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS learning_progress (
                progress_id INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id TEXT,
                skill_id TEXT,
                mastery_level REAL,
                confidence_score REAL,
                time_spent INTEGER,
                attempts INTEGER,
                last_practice TIMESTAMP,
                learning_velocity REAL,
                retention_rate REAL,
                difficulty_preference REAL,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(student_id, skill_id)
            )
        ''')
        
        # Adaptive recommendations table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS adaptive_recommendations (
                recommendation_id TEXT PRIMARY KEY,
                student_id TEXT,
                skill_id TEXT,
                project_id INTEGER,
                difficulty_adjustment REAL,
                estimated_duration INTEGER,
                rationale TEXT,
                confidence REAL,
                priority REAL,
                adaptive_features TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                used_at TIMESTAMP,
                effectiveness_score REAL
            )
        ''')
        
        # Curriculum paths table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS curriculum_paths (
                path_id TEXT PRIMARY KEY,
                student_id TEXT,
                path_data TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Learning sessions table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS learning_sessions (
                session_id INTEGER PRIMARY KEY AUTOINCREMENT,
                student_id TEXT,
                skill_id TEXT,
                project_id INTEGER,
                start_time TIMESTAMP,
                end_time TIMESTAMP,
                performance_score REAL,
                engagement_metrics TEXT,
                adaptive_adjustments TEXT
            )
        ''')
        
        conn.commit()
        conn.close()

    def init_skill_graph(self):
        """Initialize the skill dependency graph"""
        self.skill_graph = {
            "basic_c": SkillNode(
                skill_id="basic_c",
                name="Basic C Programming",
                category=SkillCategory.FUNDAMENTALS,
                difficulty_level=1,
                prerequisites=[],
                learning_objectives=[LearningObjective.UNDERSTANDING, LearningObjective.APPLICATION],
                estimated_hours=20,
                projects=[1],
                concepts=["variables", "functions", "control_structures", "data_types"],
                assessment_criteria={"syntax": 0.3, "logic": 0.4, "style": 0.3},
                industry_relevance=0.9
            ),
            
            "gpio_control": SkillNode(
                skill_id="gpio_control",
                name="GPIO Control and Digital I/O",
                category=SkillCategory.HARDWARE_INTERFACE,
                difficulty_level=2,
                prerequisites=["basic_c"],
                learning_objectives=[LearningObjective.UNDERSTANDING, LearningObjective.APPLICATION],
                estimated_hours=15,
                projects=[1, 2],
                concepts=["digital_io", "pin_configuration", "register_manipulation"],
                assessment_criteria={"hardware_understanding": 0.4, "implementation": 0.4, "debugging": 0.2},
                industry_relevance=0.95
            ),
            
            "timer_pwm": SkillNode(
                skill_id="timer_pwm",
                name="Timers and PWM Generation",
                category=SkillCategory.HARDWARE_INTERFACE,
                difficulty_level=3,
                prerequisites=["gpio_control"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS],
                estimated_hours=20,
                projects=[2, 7],
                concepts=["timer_registers", "pwm_generation", "frequency_calculation"],
                assessment_criteria={"timing_accuracy": 0.4, "calculation": 0.3, "optimization": 0.3},
                industry_relevance=0.9
            ),
            
            "adc_sensors": SkillNode(
                skill_id="adc_sensors",
                name="ADC and Sensor Interfacing",
                category=SkillCategory.HARDWARE_INTERFACE,
                difficulty_level=3,
                prerequisites=["gpio_control"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS],
                estimated_hours=18,
                projects=[3, 9],
                concepts=["analog_conversion", "sensor_calibration", "signal_conditioning"],
                assessment_criteria={"accuracy": 0.4, "calibration": 0.3, "filtering": 0.3},
                industry_relevance=0.85
            ),
            
            "uart_communication": SkillNode(
                skill_id="uart_communication",
                name="UART Serial Communication",
                category=SkillCategory.COMMUNICATION,
                difficulty_level=2,
                prerequisites=["gpio_control"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS],
                estimated_hours=16,
                projects=[4, 9],
                concepts=["serial_protocol", "baud_rate", "data_framing"],
                assessment_criteria={"protocol_understanding": 0.4, "implementation": 0.4, "error_handling": 0.2},
                industry_relevance=0.8
            ),
            
            "interrupt_handling": SkillNode(
                skill_id="interrupt_handling",
                name="Interrupt-Driven Programming",
                category=SkillCategory.REAL_TIME,
                difficulty_level=4,
                prerequisites=["timer_pwm", "gpio_control"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS, LearningObjective.SYNTHESIS],
                estimated_hours=25,
                projects=[5, 7],
                concepts=["interrupt_vectors", "isr_design", "priority_handling"],
                assessment_criteria={"responsiveness": 0.3, "efficiency": 0.3, "safety": 0.4},
                industry_relevance=0.95
            ),
            
            "i2c_protocol": SkillNode(
                skill_id="i2c_protocol",
                name="I2C Communication Protocol",
                category=SkillCategory.COMMUNICATION,
                difficulty_level=4,
                prerequisites=["uart_communication", "interrupt_handling"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS],
                estimated_hours=22,
                projects=[6],
                concepts=["i2c_protocol", "master_slave", "multi_device"],
                assessment_criteria={"protocol_mastery": 0.4, "multi_device": 0.3, "error_recovery": 0.3},
                industry_relevance=0.8
            ),
            
            "spi_protocol": SkillNode(
                skill_id="spi_protocol",
                name="SPI High-Speed Communication",
                category=SkillCategory.COMMUNICATION,
                difficulty_level=4,
                prerequisites=["uart_communication"],
                learning_objectives=[LearningObjective.APPLICATION, LearningObjective.ANALYSIS],
                estimated_hours=20,
                projects=[8],
                concepts=["spi_protocol", "high_speed_data", "synchronous_communication"],
                assessment_criteria={"speed_optimization": 0.4, "data_integrity": 0.3, "timing": 0.3},
                industry_relevance=0.75
            ),
            
            "rtos_concepts": SkillNode(
                skill_id="rtos_concepts",
                name="Real-Time Operating Systems",
                category=SkillCategory.REAL_TIME,
                difficulty_level=5,
                prerequisites=["interrupt_handling", "timer_pwm"],
                learning_objectives=[LearningObjective.SYNTHESIS, LearningObjective.EVALUATION],
                estimated_hours=30,
                projects=[10],
                concepts=["task_scheduling", "semaphores", "message_queues"],
                assessment_criteria={"system_design": 0.4, "timing_analysis": 0.3, "resource_management": 0.3},
                industry_relevance=0.9
            ),
            
            "performance_optimization": SkillNode(
                skill_id="performance_optimization",
                name="Code and System Optimization",
                category=SkillCategory.OPTIMIZATION,
                difficulty_level=5,
                prerequisites=["interrupt_handling", "adc_sensors"],
                learning_objectives=[LearningObjective.ANALYSIS, LearningObjective.EVALUATION],
                estimated_hours=25,
                projects=[9, 10],
                concepts=["code_optimization", "memory_management", "power_efficiency"],
                assessment_criteria={"efficiency_improvement": 0.4, "resource_usage": 0.3, "maintainability": 0.3},
                industry_relevance=0.95
            )
        }

    def analyze_student_progress(self, student_id: str) -> Dict[str, LearningProgress]:
        """Analyze current learning progress for a student"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT * FROM learning_progress WHERE student_id = ?
        ''', (student_id,))
        
        progress_data = {}
        for row in cursor.fetchall():
            progress = LearningProgress(
                student_id=row[1],
                skill_id=row[2],
                mastery_level=row[3],
                confidence_score=row[4],
                time_spent=row[5],
                attempts=row[6],
                last_practice=datetime.fromisoformat(row[7]) if row[7] else datetime.now(),
                learning_velocity=row[8],
                retention_rate=row[9],
                difficulty_preference=row[10]
            )
            progress_data[progress.skill_id] = progress
        
        conn.close()
        return progress_data

    def calculate_skill_readiness(self, student_id: str, skill_id: str) -> float:
        """Calculate readiness score for a specific skill"""
        skill = self.skill_graph[skill_id]
        progress_data = self.analyze_student_progress(student_id)
        
        # Check prerequisite mastery
        prerequisite_readiness = 1.0
        if skill.prerequisites:
            prerequisite_scores = []
            for prereq in skill.prerequisites:
                if prereq in progress_data:
                    prereq_progress = progress_data[prereq]
                    # Apply forgetting curve
                    days_since_practice = (datetime.now() - prereq_progress.last_practice).days
                    retention_factor = math.exp(-days_since_practice * 0.1)
                    effective_mastery = prereq_progress.mastery_level * retention_factor
                    prerequisite_scores.append(effective_mastery)
                else:
                    prerequisite_scores.append(0.0)
            prerequisite_readiness = min(prerequisite_scores) if prerequisite_scores else 0.0
        
        # Factor in current skill progress
        current_progress = 0.0
        if skill_id in progress_data:
            current_progress = progress_data[skill_id].mastery_level
        
        # Calculate overall readiness
        readiness = (prerequisite_readiness * 0.7) + (current_progress * 0.3)
        
        return min(readiness, 1.0)

    def generate_adaptive_recommendations(self, student_id: str, 
                                        max_recommendations: int = 5) -> List[AdaptiveRecommendation]:
        """Generate personalized learning recommendations"""
        progress_data = self.analyze_student_progress(student_id)
        recommendations = []
        
        # Analyze learning patterns
        learning_patterns = self._analyze_learning_patterns(student_id, progress_data)
        
        # Find skills ready for learning
        ready_skills = []
        for skill_id, skill in self.skill_graph.items():
            readiness = self.calculate_skill_readiness(student_id, skill_id)
            if readiness >= 0.7 and skill_id not in progress_data:
                ready_skills.append((skill_id, readiness))
        
        # Sort by readiness and industry relevance
        ready_skills.sort(key=lambda x: (x[1], self.skill_graph[x[0]].industry_relevance), reverse=True)
        
        # Generate recommendations for top skills
        for skill_id, readiness in ready_skills[:max_recommendations]:
            skill = self.skill_graph[skill_id]
            
            # Determine optimal project
            best_project = self._select_optimal_project(skill_id, learning_patterns)
            
            # Calculate difficulty adjustment
            difficulty_adjustment = self._calculate_difficulty_adjustment(
                student_id, skill_id, learning_patterns
            )
            
            # Estimate duration based on learning velocity
            estimated_duration = self._estimate_learning_duration(
                skill_id, learning_patterns
            )
            
            # Generate rationale
            rationale = self._generate_recommendation_rationale(
                skill_id, readiness, learning_patterns
            )
            
            # Determine adaptive features
            adaptive_features = self._determine_adaptive_features(
                skill_id, learning_patterns
            )
            
            recommendation = AdaptiveRecommendation(
                recommendation_id=f"rec_{student_id}_{skill_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}",
                student_id=student_id,
                skill_id=skill_id,
                project_id=best_project,
                difficulty_adjustment=difficulty_adjustment,
                estimated_duration=estimated_duration,
                rationale=rationale,
                confidence=readiness * 0.8 + 0.2,
                priority=readiness * skill.industry_relevance,
                adaptive_features=adaptive_features
            )
            
            recommendations.append(recommendation)
        
        # Save recommendations
        self._save_recommendations(recommendations)
        
        return recommendations

    def create_adaptive_curriculum_path(self, student_id: str, 
                                      target_certification: str = "ESD") -> CurriculumPath:
        """Create personalized curriculum path with adaptive sequencing"""
        
        # Define certification skill requirements
        certification_skills = self._get_certification_skills(target_certification)
        
        # Analyze current progress
        progress_data = self.analyze_student_progress(student_id)
        learning_patterns = self._analyze_learning_patterns(student_id, progress_data)
        
        # Determine optimal skill sequence using topological sort with adaptations
        skills_sequence = self._adaptive_topological_sort(
            certification_skills, progress_data, learning_patterns
        )
        
        # Create milestones
        milestones = self._create_adaptive_milestones(skills_sequence, learning_patterns)
        
        # Calculate completion percentage
        mastered_skills = [skill for skill, progress in progress_data.items() 
                          if progress.mastery_level >= self.mastery_threshold]
        completion_percentage = len(mastered_skills) / len(certification_skills) * 100
        
        # Estimate completion date
        estimated_completion = self._estimate_completion_date(
            skills_sequence, progress_data, learning_patterns
        )
        
        # Generate adaptive adjustments
        adaptive_adjustments = self._generate_adaptive_adjustments(learning_patterns)
        
        # Generate learning style adaptations
        learning_style_adaptations = self._generate_learning_style_adaptations(learning_patterns)
        
        path = CurriculumPath(
            path_id=f"path_{student_id}_{target_certification}_{datetime.now().strftime('%Y%m%d')}",
            student_id=student_id,
            target_certification=target_certification,
            current_phase=self._determine_current_phase(completion_percentage),
            skills_sequence=skills_sequence,
            milestones=milestones,
            completion_percentage=completion_percentage,
            estimated_completion=estimated_completion,
            adaptive_adjustments=adaptive_adjustments,
            learning_style_adaptations=learning_style_adaptations
        )
        
        # Save curriculum path
        self._save_curriculum_path(path)
        
        return path

    def update_learning_progress(self, student_id: str, skill_id: str, 
                               session_data: Dict[str, any]):
        """Update learning progress based on session performance"""
        
        # Calculate performance metrics
        performance_score = session_data.get('performance_score', 0.0)
        time_spent = session_data.get('duration_minutes', 0)
        engagement_metrics = session_data.get('engagement_metrics', {})
        
        # Get current progress
        progress_data = self.analyze_student_progress(student_id)
        current_progress = progress_data.get(skill_id)
        
        if current_progress:
            # Update existing progress
            new_mastery = self._calculate_new_mastery_level(
                current_progress, performance_score, time_spent
            )
            new_confidence = self._calculate_confidence_score(
                current_progress, performance_score, engagement_metrics
            )
            new_velocity = self._calculate_learning_velocity(
                current_progress, performance_score, time_spent
            )
            new_retention = self._calculate_retention_rate(
                current_progress, session_data
            )
            new_difficulty_preference = self._update_difficulty_preference(
                current_progress, session_data
            )
            
            # Update database
            self._update_progress_record(
                student_id, skill_id, new_mastery, new_confidence,
                current_progress.time_spent + time_spent,
                current_progress.attempts + 1,
                new_velocity, new_retention, new_difficulty_preference
            )
        else:
            # Create new progress record
            initial_mastery = max(0.1, performance_score * 0.8)
            initial_confidence = max(0.1, performance_score * 0.7)
            
            self._create_progress_record(
                student_id, skill_id, initial_mastery, initial_confidence,
                time_spent, 1, 0.5, 1.0, 0.0
            )
        
        # Save session data
        self._save_learning_session(student_id, skill_id, session_data)

    def _analyze_learning_patterns(self, student_id: str, 
                                 progress_data: Dict[str, LearningProgress]) -> Dict[str, any]:
        """Analyze student's learning patterns"""
        patterns = {
            "avg_learning_velocity": 0.0,
            "preferred_difficulty": 0.0,
            "learning_style_indicators": {},
            "optimal_session_length": 60,  # minutes
            "engagement_patterns": {},
            "skill_preferences": []
        }
        
        if progress_data:
            velocities = [p.learning_velocity for p in progress_data.values() if p.learning_velocity > 0]
            if velocities:
                patterns["avg_learning_velocity"] = sum(velocities) / len(velocities)
            
            difficulties = [p.difficulty_preference for p in progress_data.values()]
            if difficulties:
                patterns["preferred_difficulty"] = sum(difficulties) / len(difficulties)
            
            # Identify skill category preferences
            category_performance = {}
            for skill_id, progress in progress_data.items():
                if skill_id in self.skill_graph:
                    category = self.skill_graph[skill_id].category
                    if category not in category_performance:
                        category_performance[category] = []
                    category_performance[category].append(progress.mastery_level)
            
            patterns["skill_preferences"] = [
                category.value for category, scores in category_performance.items()
                if sum(scores) / len(scores) > 0.7
            ]
        
        return patterns

    def _select_optimal_project(self, skill_id: str, learning_patterns: Dict[str, any]) -> int:
        """Select the most suitable project for learning a skill"""
        skill = self.skill_graph[skill_id]
        available_projects = skill.projects
        
        if not available_projects:
            return 1  # Default project
        
        # Select based on difficulty preference and learning patterns
        difficulty_preference = learning_patterns.get("preferred_difficulty", 0.0)
        
        if difficulty_preference > 0.3:  # Prefers harder challenges
            return max(available_projects)
        elif difficulty_preference < -0.3:  # Prefers easier tasks
            return min(available_projects)
        else:  # Balanced approach
            return available_projects[len(available_projects) // 2]

    def _calculate_difficulty_adjustment(self, student_id: str, skill_id: str, 
                                       learning_patterns: Dict[str, any]) -> float:
        """Calculate optimal difficulty adjustment for the student"""
        base_difficulty = self.skill_graph[skill_id].difficulty_level / 5.0
        
        # Adjust based on learning velocity
        velocity_factor = learning_patterns.get("avg_learning_velocity", 0.5)
        if velocity_factor > 0.7:
            difficulty_adjustment = 0.2  # Increase difficulty for fast learners
        elif velocity_factor < 0.3:
            difficulty_adjustment = -0.3  # Decrease difficulty for slower learners
        else:
            difficulty_adjustment = 0.0
        
        # Factor in difficulty preference
        preference_factor = learning_patterns.get("preferred_difficulty", 0.0)
        difficulty_adjustment += preference_factor * 0.2
        
        return max(-0.5, min(0.5, difficulty_adjustment))

    def _estimate_learning_duration(self, skill_id: str, 
                                  learning_patterns: Dict[str, any]) -> int:
        """Estimate learning duration in minutes"""
        skill = self.skill_graph[skill_id]
        base_hours = skill.estimated_hours
        
        # Adjust based on learning velocity
        velocity_factor = learning_patterns.get("avg_learning_velocity", 0.5)
        adjusted_hours = base_hours / max(0.3, velocity_factor)
        
        # Convert to minutes and add buffer
        estimated_minutes = int(adjusted_hours * 60 * 1.2)  # 20% buffer
        
        return estimated_minutes

    def _generate_recommendation_rationale(self, skill_id: str, readiness: float, 
                                         learning_patterns: Dict[str, any]) -> str:
        """Generate explanation for the recommendation"""
        skill = self.skill_graph[skill_id]
        
        rationale_parts = []
        
        if readiness > 0.9:
            rationale_parts.append("You're fully prepared for this skill")
        elif readiness > 0.7:
            rationale_parts.append("You have solid foundation for this skill")
        else:
            rationale_parts.append("This skill builds on your current knowledge")
        
        # Add industry relevance
        if skill.industry_relevance > 0.9:
            rationale_parts.append("highly valued in industry")
        elif skill.industry_relevance > 0.7:
            rationale_parts.append("important for professional development")
        
        # Add learning pattern insights
        velocity = learning_patterns.get("avg_learning_velocity", 0.5)
        if velocity > 0.7:
            rationale_parts.append("matches your fast learning pace")
        elif velocity < 0.3:
            rationale_parts.append("appropriate for steady learning approach")
        
        return ". ".join(rationale_parts).capitalize() + "."

    def _determine_adaptive_features(self, skill_id: str, 
                                   learning_patterns: Dict[str, any]) -> List[str]:
        """Determine adaptive features for the learning experience"""
        features = []
        
        # Learning velocity adaptations
        velocity = learning_patterns.get("avg_learning_velocity", 0.5)
        if velocity > 0.7:
            features.extend(["accelerated_pace", "additional_challenges", "peer_mentoring"])
        elif velocity < 0.3:
            features.extend(["guided_steps", "extra_examples", "frequent_checkpoints"])
        
        # Difficulty preference adaptations
        difficulty_pref = learning_patterns.get("preferred_difficulty", 0.0)
        if difficulty_pref > 0.3:
            features.extend(["advanced_scenarios", "open_ended_problems"])
        elif difficulty_pref < -0.3:
            features.extend(["structured_guidance", "step_by_step_tutorials"])
        
        # Skill category preferences
        preferences = learning_patterns.get("skill_preferences", [])
        skill_category = self.skill_graph[skill_id].category.value
        if skill_category in preferences:
            features.append("preferred_domain_focus")
        else:
            features.append("domain_bridging_support")
        
        return features

    def _get_certification_skills(self, certification: str) -> List[str]:
        """Get required skills for certification level"""
        skill_maps = {
            "AED": ["basic_c", "gpio_control", "timer_pwm", "adc_sensors", "uart_communication"],
            "ESD": ["basic_c", "gpio_control", "timer_pwm", "adc_sensors", "uart_communication", 
                   "interrupt_handling", "i2c_protocol"],
            "SEE": ["basic_c", "gpio_control", "timer_pwm", "adc_sensors", "uart_communication",
                   "interrupt_handling", "i2c_protocol", "spi_protocol", "performance_optimization"],
            "ESA": list(self.skill_graph.keys())  # All skills
        }
        
        return skill_maps.get(certification, skill_maps["AED"])

    def _adaptive_topological_sort(self, target_skills: List[str], 
                                 progress_data: Dict[str, LearningProgress],
                                 learning_patterns: Dict[str, any]) -> List[str]:
        """Perform adaptive topological sort considering learning patterns"""
        
        # Build dependency graph
        in_degree = {skill: 0 for skill in target_skills}
        adj_list = {skill: [] for skill in target_skills}
        
        for skill_id in target_skills:
            if skill_id in self.skill_graph:
                skill = self.skill_graph[skill_id]
                for prereq in skill.prerequisites:
                    if prereq in target_skills:
                        adj_list[prereq].append(skill_id)
                        in_degree[skill_id] += 1
        
        # Priority queue considering learning patterns
        queue = []
        for skill in target_skills:
            if in_degree[skill] == 0:
                priority = self._calculate_skill_priority(skill, learning_patterns)
                queue.append((priority, skill))
        
        queue.sort(reverse=True)  # Higher priority first
        result = []
        
        while queue:
            _, current_skill = queue.pop(0)
            result.append(current_skill)
            
            # Update dependencies
            for neighbor in adj_list[current_skill]:
                in_degree[neighbor] -= 1
                if in_degree[neighbor] == 0:
                    priority = self._calculate_skill_priority(neighbor, learning_patterns)
                    queue.append((priority, neighbor))
            
            queue.sort(reverse=True)
        
        return result

    def _calculate_skill_priority(self, skill_id: str, learning_patterns: Dict[str, any]) -> float:
        """Calculate priority score for skill sequencing"""
        if skill_id not in self.skill_graph:
            return 0.0
        
        skill = self.skill_graph[skill_id]
        priority = skill.industry_relevance
        
        # Boost priority for preferred skill categories
        preferences = learning_patterns.get("skill_preferences", [])
        if skill.category.value in preferences:
            priority += 0.2
        
        # Adjust for difficulty preference
        difficulty_pref = learning_patterns.get("preferred_difficulty", 0.0)
        normalized_difficulty = (skill.difficulty_level - 1) / 4.0  # 0-1 scale
        if difficulty_pref > 0 and normalized_difficulty > 0.6:
            priority += 0.1
        elif difficulty_pref < 0 and normalized_difficulty < 0.4:
            priority += 0.1
        
        return priority

    def _create_adaptive_milestones(self, skills_sequence: List[str], 
                                  learning_patterns: Dict[str, any]) -> List[Dict[str, any]]:
        """Create adaptive milestones based on skill sequence"""
        milestones = []
        
        # Group skills into phases
        phase_size = max(2, len(skills_sequence) // 4)
        
        for i in range(0, len(skills_sequence), phase_size):
            phase_skills = skills_sequence[i:i+phase_size]
            
            milestone = {
                "phase": f"Phase {len(milestones) + 1}",
                "skills": phase_skills,
                "estimated_hours": sum(self.skill_graph[skill].estimated_hours 
                                     for skill in phase_skills if skill in self.skill_graph),
                "objectives": self._get_phase_objectives(phase_skills),
                "assessment_criteria": self._get_phase_assessment_criteria(phase_skills),
                "adaptive_features": self._get_phase_adaptive_features(phase_skills, learning_patterns)
            }
            
            milestones.append(milestone)
        
        return milestones

    def _estimate_completion_date(self, skills_sequence: List[str], 
                                progress_data: Dict[str, LearningProgress],
                                learning_patterns: Dict[str, any]) -> datetime:
        """Estimate curriculum completion date"""
        
        total_hours = 0
        for skill_id in skills_sequence:
            if skill_id in self.skill_graph:
                if skill_id not in progress_data:  # Not yet started
                    skill_hours = self.skill_graph[skill_id].estimated_hours
                    # Adjust for learning velocity
                    velocity = learning_patterns.get("avg_learning_velocity", 0.5)
                    adjusted_hours = skill_hours / max(0.3, velocity)
                    total_hours += adjusted_hours
        
        # Assume 10 hours of study per week
        weeks_needed = total_hours / 10
        return datetime.now() + timedelta(weeks=weeks_needed)

    def _generate_adaptive_adjustments(self, learning_patterns: Dict[str, any]) -> List[str]:
        """Generate adaptive adjustments for the curriculum"""
        adjustments = []
        
        velocity = learning_patterns.get("avg_learning_velocity", 0.5)
        if velocity > 0.7:
            adjustments.append("Accelerated progression with bonus challenges")
        elif velocity < 0.3:
            adjustments.append("Extended practice time with additional support")
        
        difficulty_pref = learning_patterns.get("preferred_difficulty", 0.0)
        if difficulty_pref > 0.3:
            adjustments.append("Enhanced problem complexity and real-world scenarios")
        elif difficulty_pref < -0.3:
            adjustments.append("Structured guidance with incremental difficulty increase")
        
        return adjustments

    def _generate_learning_style_adaptations(self, learning_patterns: Dict[str, any]) -> List[str]:
        """Generate learning style specific adaptations"""
        adaptations = []
        
        # Based on engagement patterns and preferences
        style_indicators = learning_patterns.get("learning_style_indicators", {})
        
        if style_indicators.get("visual_preference", False):
            adaptations.extend(["Circuit diagrams and flowcharts", "Visual debugging tools"])
        
        if style_indicators.get("hands_on_preference", False):
            adaptations.extend(["Hardware simulation priority", "Physical prototyping exercises"])
        
        if style_indicators.get("theory_preference", False):
            adaptations.extend(["Detailed explanations", "Mathematical analysis focus"])
        
        return adaptations

    def _determine_current_phase(self, completion_percentage: float) -> str:
        """Determine current curriculum phase"""
        if completion_percentage < 25:
            return "Foundation"
        elif completion_percentage < 50:
            return "Development"
        elif completion_percentage < 75:
            return "Advanced"
        else:
            return "Mastery"

    # Additional helper methods for calculations and database operations
    def _calculate_new_mastery_level(self, current_progress: LearningProgress, 
                                   performance_score: float, time_spent: int) -> float:
        """Calculate updated mastery level"""
        learning_gain = performance_score * 0.1 * (time_spent / 60.0)  # Per hour
        decay_factor = 0.02  # Small decay to encourage regular practice
        
        new_mastery = current_progress.mastery_level + learning_gain - decay_factor
        return max(0.0, min(1.0, new_mastery))

    def _calculate_confidence_score(self, current_progress: LearningProgress, 
                                  performance_score: float, 
                                  engagement_metrics: Dict[str, any]) -> float:
        """Calculate confidence score based on performance and engagement"""
        confidence_gain = performance_score * 0.05
        engagement_factor = engagement_metrics.get("focus_score", 0.5)
        
        new_confidence = current_progress.confidence_score + confidence_gain * engagement_factor
        return max(0.0, min(1.0, new_confidence))

    def _calculate_learning_velocity(self, current_progress: LearningProgress, 
                                   performance_score: float, time_spent: int) -> float:
        """Calculate learning velocity (mastery gain per hour)"""
        if time_spent == 0:
            return current_progress.learning_velocity
        
        session_velocity = performance_score / (time_spent / 60.0)
        
        # Exponential moving average
        alpha = 0.3
        new_velocity = alpha * session_velocity + (1 - alpha) * current_progress.learning_velocity
        
        return max(0.1, min(2.0, new_velocity))

    def _calculate_retention_rate(self, current_progress: LearningProgress, 
                                session_data: Dict[str, any]) -> float:
        """Calculate knowledge retention rate"""
        days_since_last = (datetime.now() - current_progress.last_practice).days
        
        if days_since_last == 0:
            return current_progress.retention_rate
        
        # Forgetting curve with performance factor
        performance_score = session_data.get('performance_score', 0.5)
        retention_factor = math.exp(-days_since_last * 0.1) * performance_score
        
        # Update retention rate
        new_retention = 0.7 * current_progress.retention_rate + 0.3 * retention_factor
        return max(0.1, min(1.0, new_retention))

    def _update_difficulty_preference(self, current_progress: LearningProgress, 
                                    session_data: Dict[str, any]) -> float:
        """Update difficulty preference based on session feedback"""
        difficulty_rating = session_data.get('difficulty_rating', 0)  # -1 to 1
        engagement = session_data.get('engagement_metrics', {}).get('satisfaction', 0.5)
        
        # Adjust preference based on engagement with difficulty
        if engagement > 0.7 and difficulty_rating > 0:
            adjustment = 0.1  # Increase preference for difficulty
        elif engagement < 0.3 and difficulty_rating > 0:
            adjustment = -0.1  # Decrease preference for difficulty
        else:
            adjustment = 0.0
        
        new_preference = current_progress.difficulty_preference + adjustment
        return max(-1.0, min(1.0, new_preference))

    def _save_recommendations(self, recommendations: List[AdaptiveRecommendation]):
        """Save recommendations to database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        for rec in recommendations:
            cursor.execute('''
                INSERT INTO adaptive_recommendations 
                (recommendation_id, student_id, skill_id, project_id, difficulty_adjustment,
                 estimated_duration, rationale, confidence, priority, adaptive_features)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (rec.recommendation_id, rec.student_id, rec.skill_id, rec.project_id,
                  rec.difficulty_adjustment, rec.estimated_duration, rec.rationale,
                  rec.confidence, rec.priority, json.dumps(rec.adaptive_features)))
        
        conn.commit()
        conn.close()

    def _save_curriculum_path(self, path: CurriculumPath):
        """Save curriculum path to database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO curriculum_paths (path_id, student_id, path_data, updated_at)
            VALUES (?, ?, ?, CURRENT_TIMESTAMP)
        ''', (path.path_id, path.student_id, json.dumps(asdict(path), default=str)))
        
        conn.commit()
        conn.close()

    def _update_progress_record(self, student_id: str, skill_id: str, mastery_level: float,
                              confidence_score: float, time_spent: int, attempts: int,
                              learning_velocity: float, retention_rate: float,
                              difficulty_preference: float):
        """Update progress record in database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            UPDATE learning_progress 
            SET mastery_level = ?, confidence_score = ?, time_spent = ?, attempts = ?,
                last_practice = CURRENT_TIMESTAMP, learning_velocity = ?, retention_rate = ?,
                difficulty_preference = ?, updated_at = CURRENT_TIMESTAMP
            WHERE student_id = ? AND skill_id = ?
        ''', (mastery_level, confidence_score, time_spent, attempts, learning_velocity,
              retention_rate, difficulty_preference, student_id, skill_id))
        
        conn.commit()
        conn.close()

    def _create_progress_record(self, student_id: str, skill_id: str, mastery_level: float,
                              confidence_score: float, time_spent: int, attempts: int,
                              learning_velocity: float, retention_rate: float,
                              difficulty_preference: float):
        """Create new progress record in database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO learning_progress 
            (student_id, skill_id, mastery_level, confidence_score, time_spent, attempts,
             last_practice, learning_velocity, retention_rate, difficulty_preference)
            VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?, ?)
        ''', (student_id, skill_id, mastery_level, confidence_score, time_spent, attempts,
              learning_velocity, retention_rate, difficulty_preference))
        
        conn.commit()
        conn.close()

    def _save_learning_session(self, student_id: str, skill_id: str, session_data: Dict[str, any]):
        """Save learning session data"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO learning_sessions 
            (student_id, skill_id, project_id, start_time, end_time, performance_score,
             engagement_metrics, adaptive_adjustments)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (student_id, skill_id, session_data.get('project_id'),
              session_data.get('start_time'), session_data.get('end_time'),
              session_data.get('performance_score'),
              json.dumps(session_data.get('engagement_metrics', {})),
              json.dumps(session_data.get('adaptive_adjustments', []))))
        
        conn.commit()
        conn.close()

    def _get_phase_objectives(self, phase_skills: List[str]) -> List[str]:
        """Get learning objectives for a phase"""
        objectives = []
        for skill_id in phase_skills:
            if skill_id in self.skill_graph:
                skill = self.skill_graph[skill_id]
                objectives.extend([obj.value for obj in skill.learning_objectives])
        return list(set(objectives))

    def _get_phase_assessment_criteria(self, phase_skills: List[str]) -> Dict[str, float]:
        """Get assessment criteria for a phase"""
        criteria = {}
        for skill_id in phase_skills:
            if skill_id in self.skill_graph:
                skill = self.skill_graph[skill_id]
                for criterion, weight in skill.assessment_criteria.items():
                    criteria[criterion] = criteria.get(criterion, 0) + weight
        
        # Normalize weights
        total_weight = sum(criteria.values())
        if total_weight > 0:
            criteria = {k: v/total_weight for k, v in criteria.items()}
        
        return criteria

    def _get_phase_adaptive_features(self, phase_skills: List[str], 
                                   learning_patterns: Dict[str, any]) -> List[str]:
        """Get adaptive features for a phase"""
        features = []
        
        # Check if phase focuses on specific categories
        categories = set()
        for skill_id in phase_skills:
            if skill_id in self.skill_graph:
                categories.add(self.skill_graph[skill_id].category)
        
        if SkillCategory.HARDWARE_INTERFACE in categories:
            features.append("Hardware simulation priority")
        if SkillCategory.COMMUNICATION in categories:
            features.append("Protocol analyzer tools")
        if SkillCategory.REAL_TIME in categories:
            features.append("Timing analysis tools")
        
        return features

# Example usage
if __name__ == "__main__":
    # Initialize adaptive curriculum engine
    engine = AdaptiveCurriculumEngine()
    
    # Example student progress data
    student_id = "student_001"
    
    # Simulate some learning progress
    session_data_1 = {
        'performance_score': 0.8,
        'duration_minutes': 90,
        'engagement_metrics': {'focus_score': 0.9, 'satisfaction': 0.8},
        'difficulty_rating': 0.3,
        'project_id': 1
    }
    
    session_data_2 = {
        'performance_score': 0.9,
        'duration_minutes': 75,
        'engagement_metrics': {'focus_score': 0.85, 'satisfaction': 0.9},
        'difficulty_rating': 0.5,
        'project_id': 2
    }
    
    # Update progress for some skills
    engine.update_learning_progress(student_id, "basic_c", session_data_1)
    engine.update_learning_progress(student_id, "gpio_control", session_data_2)
    
    print("🧠 Adaptive Curriculum Engine Demo")
    print("=" * 50)
    
    # Analyze current progress
    progress = engine.analyze_student_progress(student_id)
    print(f"\n📊 Current Progress for {student_id}:")
    for skill_id, prog in progress.items():
        print(f"  {skill_id}: {prog.mastery_level:.2f} mastery, {prog.confidence_score:.2f} confidence")
    
    # Calculate skill readiness
    print(f"\n🎯 Skill Readiness Scores:")
    for skill_id in ["timer_pwm", "adc_sensors", "uart_communication"]:
        readiness = engine.calculate_skill_readiness(student_id, skill_id)
        print(f"  {skill_id}: {readiness:.2f}")
    
    # Generate adaptive recommendations
    recommendations = engine.generate_adaptive_recommendations(student_id)
    print(f"\n💡 Adaptive Recommendations ({len(recommendations)} found):")
    for i, rec in enumerate(recommendations, 1):
        print(f"  {i}. {rec.skill_id} (Project {rec.project_id})")
        print(f"     Priority: {rec.priority:.2f}, Confidence: {rec.confidence:.2f}")
        print(f"     Duration: {rec.estimated_duration} minutes")
        print(f"     Rationale: {rec.rationale}")
        print(f"     Adaptive features: {', '.join(rec.adaptive_features)}")
        print()
    
    # Create adaptive curriculum path
    curriculum_path = engine.create_adaptive_curriculum_path(student_id, "ESD")
    print(f"\n🛤️  Adaptive Curriculum Path: {curriculum_path.path_id}")
    print(f"   Target: {curriculum_path.target_certification}")
    print(f"   Current Phase: {curriculum_path.current_phase}")
    print(f"   Completion: {curriculum_path.completion_percentage:.1f}%")
    print(f"   Skills Sequence: {' → '.join(curriculum_path.skills_sequence[:5])}...")
    print(f"   Estimated Completion: {curriculum_path.estimated_completion.strftime('%Y-%m-%d')}")
    
    print(f"\n   📈 Adaptive Adjustments:")
    for adjustment in curriculum_path.adaptive_adjustments:
        print(f"   • {adjustment}")
    
    print(f"\n   🎨 Learning Style Adaptations:")
    for adaptation in curriculum_path.learning_style_adaptations:
        print(f"   • {adaptation}")
    
    print(f"\n   🏁 Milestones:")
    for milestone in curriculum_path.milestones:
        print(f"   • {milestone['phase']}: {', '.join(milestone['skills'])}")
        print(f"     Estimated hours: {milestone['estimated_hours']}")
    
    print("\n✅ Adaptive Curriculum Engine demonstration completed!")