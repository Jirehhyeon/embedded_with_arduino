#!/usr/bin/env python3
"""
AI Assistant Integration API for Embedded Projects Platform
Description: RESTful API for seamless integration with web dashboard and development tools
Version: 1.0
Author: Embedded Projects Platform Team
"""

from flask import Flask, request, jsonify, session
from flask_cors import CORS
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
import json
import sqlite3
from datetime import datetime, timedelta
from typing import Dict, List, Optional
import hashlib
import jwt
import os
from functools import wraps

# Import AI assistant modules
from learning_assistant import AILearningAssistant, StudentProfile, LearningPath
from intelligent_code_review import IntelligentCodeReviewer, CodeReviewReport
from adaptive_curriculum import AdaptiveCurriculumEngine, AdaptiveRecommendation, CurriculumPath

app = Flask(__name__)
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', 'embedded-projects-ai-assistant-dev-key')
CORS(app)

# Rate limiting
limiter = Limiter(
    app,
    key_func=get_remote_address,
    default_limits=["200 per day", "50 per hour"]
)

# Initialize AI components
learning_assistant = AILearningAssistant()
code_reviewer = IntelligentCodeReviewer()
curriculum_engine = AdaptiveCurriculumEngine()

class APIResponse:
    """Standardized API response format"""
    
    @staticmethod
    def success(data=None, message="Success"):
        return jsonify({
            "success": True,
            "message": message,
            "data": data,
            "timestamp": datetime.now().isoformat()
        })
    
    @staticmethod
    def error(message="Error", code=400, details=None):
        return jsonify({
            "success": False,
            "message": message,
            "error_code": code,
            "details": details,
            "timestamp": datetime.now().isoformat()
        }), code

def token_required(f):
    """JWT token authentication decorator"""
    @wraps(f)
    def decorated(*args, **kwargs):
        token = request.headers.get('Authorization')
        
        if not token:
            return APIResponse.error("Token is missing", 401)
        
        try:
            if token.startswith('Bearer '):
                token = token[7:]  # Remove 'Bearer ' prefix
            
            data = jwt.decode(token, app.config['SECRET_KEY'], algorithms=['HS256'])
            current_user_id = data['user_id']
        except jwt.ExpiredSignatureError:
            return APIResponse.error("Token has expired", 401)
        except jwt.InvalidTokenError:
            return APIResponse.error("Token is invalid", 401)
        
        return f(current_user_id, *args, **kwargs)
    
    return decorated

def validate_json(required_fields):
    """Validate JSON request data decorator"""
    def decorator(f):
        @wraps(f)
        def decorated(*args, **kwargs):
            if not request.is_json:
                return APIResponse.error("Request must be JSON", 400)
            
            data = request.get_json()
            missing_fields = [field for field in required_fields if field not in data]
            
            if missing_fields:
                return APIResponse.error(
                    f"Missing required fields: {', '.join(missing_fields)}", 
                    400
                )
            
            return f(*args, **kwargs)
        
        return decorated
    return decorator

# ===============================
# Authentication Endpoints
# ===============================

@app.route('/api/auth/token', methods=['POST'])
@limiter.limit("10 per minute")
def generate_token():
    """Generate JWT token for API access"""
    data = request.get_json()
    
    if not data or 'user_id' not in data:
        return APIResponse.error("User ID is required", 400)
    
    user_id = data['user_id']
    
    # Token expires in 24 hours
    token = jwt.encode({
        'user_id': user_id,
        'exp': datetime.utcnow() + timedelta(hours=24)
    }, app.config['SECRET_KEY'], algorithm='HS256')
    
    return APIResponse.success({
        'token': token,
        'expires_in': 86400  # 24 hours in seconds
    })

# ===============================
# Learning Assistant Endpoints
# ===============================

@app.route('/api/learning/profile', methods=['POST'])
@token_required
@validate_json(['name', 'assessment_results'])
def create_student_profile(current_user_id):
    """Create a new student learning profile"""
    try:
        data = request.get_json()
        
        profile = learning_assistant.create_student_profile(
            current_user_id,
            data['name'],
            data['assessment_results']
        )
        
        return APIResponse.success({
            'profile_id': profile.student_id,
            'learning_style': profile.learning_style.value,
            'current_level': profile.current_level.value,
            'strengths': profile.strengths,
            'weaknesses': profile.weaknesses
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to create profile: {str(e)}", 500)

@app.route('/api/learning/profile', methods=['GET'])
@token_required
def get_student_profile(current_user_id):
    """Get student learning profile"""
    try:
        profile = learning_assistant._load_student_profile(current_user_id)
        
        if not profile:
            return APIResponse.error("Profile not found", 404)
        
        return APIResponse.success({
            'student_id': profile.student_id,
            'name': profile.name,
            'learning_style': profile.learning_style.value,
            'current_level': profile.current_level.value,
            'completed_projects': profile.completed_projects,
            'skill_scores': profile.skill_scores,
            'strengths': profile.strengths,
            'weaknesses': profile.weaknesses,
            'total_study_time': profile.total_study_time,
            'last_activity': profile.last_activity.isoformat()
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to get profile: {str(e)}", 500)

@app.route('/api/learning/path', methods=['POST'])
@token_required
@validate_json(['target_certification'])
def create_learning_path(current_user_id):
    """Generate personalized learning path"""
    try:
        data = request.get_json()
        
        learning_path = learning_assistant.generate_personalized_learning_path(
            current_user_id,
            data['target_certification']
        )
        
        return APIResponse.success({
            'path_id': learning_path.path_id,
            'target_skills': learning_path.target_skills,
            'recommended_projects': learning_path.recommended_projects,
            'estimated_duration': learning_path.estimated_duration,
            'current_step': learning_path.current_step,
            'completion_percentage': learning_path.completion_percentage
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to create learning path: {str(e)}", 500)

@app.route('/api/learning/hint', methods=['POST'])
@token_required
@validate_json(['project_id', 'current_code'])
def get_contextual_hint(current_user_id):
    """Get intelligent hint based on context"""
    try:
        data = request.get_json()
        
        hint = learning_assistant.provide_contextual_hint(
            current_user_id,
            data['project_id'],
            data['current_code'],
            data.get('error_message', '')
        )
        
        return APIResponse.success({
            'hint': hint,
            'context': data['project_id']
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to generate hint: {str(e)}", 500)

@app.route('/api/learning/practice', methods=['POST'])
@token_required
@validate_json(['skill', 'difficulty'])
def generate_practice_problems(current_user_id):
    """Generate practice problems for specific skill"""
    try:
        data = request.get_json()
        
        problems = learning_assistant.generate_practice_problems(
            current_user_id,
            data['skill'],
            data.get('difficulty')
        )
        
        return APIResponse.success({
            'problems': problems,
            'skill': data['skill'],
            'count': len(problems)
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to generate practice problems: {str(e)}", 500)

# ===============================
# Code Review Endpoints
# ===============================

@app.route('/api/review/analyze', methods=['POST'])
@token_required
@validate_json(['file_path', 'code_content'])
@limiter.limit("30 per hour")
def analyze_code(current_user_id):
    """Perform intelligent code review"""
    try:
        data = request.get_json()
        
        report = code_reviewer.review_code_file(
            data['file_path'],
            data['code_content'],
            data.get('project_context', {})
        )
        
        # Convert report to JSON-serializable format
        issues_data = []
        for issue in report.issues:
            issues_data.append({
                'issue_id': issue.issue_id,
                'category': issue.category.value,
                'severity': issue.severity.value,
                'title': issue.title,
                'description': issue.description,
                'line_number': issue.line_number,
                'column': issue.column,
                'code_snippet': issue.code_snippet,
                'suggestion': issue.suggestion,
                'explanation': issue.explanation,
                'auto_fixable': issue.auto_fixable,
                'confidence': issue.confidence
            })
        
        metrics_data = []
        for metric in report.performance_metrics:
            metrics_data.append({
                'metric_name': metric.metric_name,
                'current_value': metric.current_value,
                'recommended_value': metric.recommended_value,
                'unit': metric.unit,
                'impact': metric.impact
            })
        
        return APIResponse.success({
            'file_path': report.file_path,
            'overall_score': report.overall_score,
            'complexity_score': report.complexity_score,
            'maintainability_index': report.maintainability_index,
            'embedded_compliance_score': report.embedded_compliance_score,
            'issues': issues_data,
            'performance_metrics': metrics_data,
            'recommendations': report.recommendations,
            'auto_fixes': report.auto_fixes,
            'timestamp': report.timestamp.isoformat()
        })
        
    except Exception as e:
        return APIResponse.error(f"Code analysis failed: {str(e)}", 500)

@app.route('/api/review/summary', methods=['POST'])
@token_required
@validate_json(['file_path', 'code_content'])
def get_review_summary(current_user_id):
    """Get concise code review summary"""
    try:
        data = request.get_json()
        
        report = code_reviewer.review_code_file(
            data['file_path'],
            data['code_content']
        )
        
        summary = code_reviewer.generate_review_summary(report)
        
        return APIResponse.success({
            'summary': summary,
            'overall_score': report.overall_score,
            'issue_count': len(report.issues),
            'critical_issues': len([i for i in report.issues if i.severity.value == 'critical']),
            'recommendations_count': len(report.recommendations)
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to generate summary: {str(e)}", 500)

# ===============================
# Adaptive Curriculum Endpoints
# ===============================

@app.route('/api/curriculum/progress', methods=['GET'])
@token_required
def get_learning_progress(current_user_id):
    """Get current learning progress"""
    try:
        progress_data = curriculum_engine.analyze_student_progress(current_user_id)
        
        # Convert to JSON-serializable format
        progress_json = {}
        for skill_id, progress in progress_data.items():
            progress_json[skill_id] = {
                'mastery_level': progress.mastery_level,
                'confidence_score': progress.confidence_score,
                'time_spent': progress.time_spent,
                'attempts': progress.attempts,
                'last_practice': progress.last_practice.isoformat(),
                'learning_velocity': progress.learning_velocity,
                'retention_rate': progress.retention_rate,
                'difficulty_preference': progress.difficulty_preference
            }
        
        return APIResponse.success({
            'student_id': current_user_id,
            'progress': progress_json,
            'total_skills': len(progress_json)
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to get progress: {str(e)}", 500)

@app.route('/api/curriculum/readiness', methods=['POST'])
@token_required
@validate_json(['skill_id'])
def check_skill_readiness(current_user_id):
    """Check readiness for specific skill"""
    try:
        data = request.get_json()
        
        readiness = curriculum_engine.calculate_skill_readiness(
            current_user_id,
            data['skill_id']
        )
        
        return APIResponse.success({
            'skill_id': data['skill_id'],
            'readiness_score': readiness,
            'ready': readiness >= 0.7
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to check readiness: {str(e)}", 500)

@app.route('/api/curriculum/recommendations', methods=['GET'])
@token_required
def get_adaptive_recommendations(current_user_id):
    """Get adaptive learning recommendations"""
    try:
        max_recommendations = request.args.get('limit', 5, type=int)
        
        recommendations = curriculum_engine.generate_adaptive_recommendations(
            current_user_id,
            max_recommendations
        )
        
        # Convert to JSON-serializable format
        recommendations_data = []
        for rec in recommendations:
            recommendations_data.append({
                'recommendation_id': rec.recommendation_id,
                'skill_id': rec.skill_id,
                'project_id': rec.project_id,
                'difficulty_adjustment': rec.difficulty_adjustment,
                'estimated_duration': rec.estimated_duration,
                'rationale': rec.rationale,
                'confidence': rec.confidence,
                'priority': rec.priority,
                'adaptive_features': rec.adaptive_features
            })
        
        return APIResponse.success({
            'recommendations': recommendations_data,
            'count': len(recommendations_data)
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to get recommendations: {str(e)}", 500)

@app.route('/api/curriculum/path', methods=['POST'])
@token_required
@validate_json(['target_certification'])
def create_curriculum_path(current_user_id):
    """Create adaptive curriculum path"""
    try:
        data = request.get_json()
        
        curriculum_path = curriculum_engine.create_adaptive_curriculum_path(
            current_user_id,
            data['target_certification']
        )
        
        return APIResponse.success({
            'path_id': curriculum_path.path_id,
            'target_certification': curriculum_path.target_certification,
            'current_phase': curriculum_path.current_phase,
            'skills_sequence': curriculum_path.skills_sequence,
            'milestones': curriculum_path.milestones,
            'completion_percentage': curriculum_path.completion_percentage,
            'estimated_completion': curriculum_path.estimated_completion.isoformat(),
            'adaptive_adjustments': curriculum_path.adaptive_adjustments,
            'learning_style_adaptations': curriculum_path.learning_style_adaptations
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to create curriculum path: {str(e)}", 500)

@app.route('/api/curriculum/session', methods=['POST'])
@token_required
@validate_json(['skill_id', 'session_data'])
def update_learning_session(current_user_id):
    """Update learning progress from session data"""
    try:
        data = request.get_json()
        
        curriculum_engine.update_learning_progress(
            current_user_id,
            data['skill_id'],
            data['session_data']
        )
        
        return APIResponse.success({
            'message': 'Learning progress updated successfully',
            'skill_id': data['skill_id']
        })
        
    except Exception as e:
        return APIResponse.error(f"Failed to update session: {str(e)}", 500)

# ===============================
# Analytics & Reporting Endpoints
# ===============================

@app.route('/api/analytics/dashboard', methods=['GET'])
@token_required
def get_analytics_dashboard(current_user_id):
    """Get comprehensive analytics dashboard data"""
    try:
        # Get learning progress
        progress_data = curriculum_engine.analyze_student_progress(current_user_id)
        
        # Calculate summary statistics
        total_skills = len(progress_data)
        mastered_skills = len([p for p in progress_data.values() if p.mastery_level >= 0.85])
        total_study_time = sum(p.time_spent for p in progress_data.values())
        avg_velocity = sum(p.learning_velocity for p in progress_data.values()) / max(total_skills, 1)
        
        # Get recent recommendations
        recommendations = curriculum_engine.generate_adaptive_recommendations(current_user_id, 3)
        
        dashboard_data = {
            'student_summary': {
                'total_skills': total_skills,
                'mastered_skills': mastered_skills,
                'mastery_percentage': (mastered_skills / max(total_skills, 1)) * 100,
                'total_study_time_hours': total_study_time / 60,
                'average_learning_velocity': avg_velocity
            },
            'skill_progress': {
                skill_id: {
                    'mastery_level': progress.mastery_level,
                    'confidence_score': progress.confidence_score
                }
                for skill_id, progress in progress_data.items()
            },
            'recent_recommendations': [
                {
                    'skill_id': rec.skill_id,
                    'priority': rec.priority,
                    'rationale': rec.rationale
                }
                for rec in recommendations[:3]
            ]
        }
        
        return APIResponse.success(dashboard_data)
        
    except Exception as e:
        return APIResponse.error(f"Failed to get dashboard data: {str(e)}", 500)

@app.route('/api/analytics/trends', methods=['GET'])
@token_required
def get_learning_trends(current_user_id):
    """Get learning trends and patterns"""
    try:
        # This would typically query historical data from learning_sessions table
        # For now, return mock trend data
        
        trends_data = {
            'velocity_trend': {
                'last_7_days': [0.6, 0.7, 0.8, 0.75, 0.9, 0.85, 0.8],
                'improvement': 12.5  # percentage
            },
            'engagement_trend': {
                'last_7_days': [0.8, 0.85, 0.9, 0.88, 0.92, 0.87, 0.89],
                'average': 0.87
            },
            'skill_mastery_progression': {
                'gpio_control': [0.2, 0.4, 0.6, 0.8, 0.9],
                'timer_pwm': [0.1, 0.3, 0.5, 0.7],
                'adc_sensors': [0.2, 0.4, 0.6]
            }
        }
        
        return APIResponse.success(trends_data)
        
    except Exception as e:
        return APIResponse.error(f"Failed to get trends: {str(e)}", 500)

# ===============================
# Health & Status Endpoints
# ===============================

@app.route('/api/health', methods=['GET'])
def health_check():
    """API health check endpoint"""
    try:
        # Test database connections
        learning_assistant._load_student_profile("health_check")
        code_reviewer.init_database()
        curriculum_engine.init_database()
        
        return APIResponse.success({
            'status': 'healthy',
            'components': {
                'learning_assistant': 'operational',
                'code_reviewer': 'operational',
                'curriculum_engine': 'operational',
                'database': 'operational'
            },
            'version': '1.0.0'
        })
        
    except Exception as e:
        return APIResponse.error(f"Health check failed: {str(e)}", 503)

@app.route('/api/info', methods=['GET'])
def api_info():
    """Get API information and capabilities"""
    return APIResponse.success({
        'name': 'Embedded Projects AI Assistant API',
        'version': '1.0.0',
        'description': 'RESTful API for AI-powered learning assistance',
        'capabilities': {
            'learning_assistance': [
                'Student profiling and assessment',
                'Personalized learning paths',
                'Contextual hints and guidance',
                'Practice problem generation'
            ],
            'code_review': [
                'Embedded systems specific analysis',
                'Security vulnerability detection',
                'Performance optimization suggestions',
                'Best practices validation'
            ],
            'adaptive_curriculum': [
                'Progress tracking and analytics',
                'Skill readiness assessment',
                'Adaptive recommendations',
                'Dynamic curriculum paths'
            ]
        },
        'endpoints': {
            'authentication': '/api/auth/*',
            'learning': '/api/learning/*',
            'code_review': '/api/review/*',
            'curriculum': '/api/curriculum/*',
            'analytics': '/api/analytics/*'
        }
    })

# ===============================
# Error Handlers
# ===============================

@app.errorhandler(404)
def not_found(error):
    return APIResponse.error("Endpoint not found", 404)

@app.errorhandler(405)
def method_not_allowed(error):
    return APIResponse.error("Method not allowed", 405)

@app.errorhandler(429)
def ratelimit_handler(e):
    return APIResponse.error(f"Rate limit exceeded: {e.description}", 429)

@app.errorhandler(500)
def internal_error(error):
    return APIResponse.error("Internal server error", 500)

# ===============================
# Development & Testing
# ===============================

if __name__ == '__main__':
    # Development server configuration
    app.run(
        host='0.0.0.0',
        port=5000,
        debug=True,
        threaded=True
    )

# Example usage for testing
"""
# Generate token
curl -X POST http://localhost:5000/api/auth/token \
  -H "Content-Type: application/json" \
  -d '{"user_id": "student_001"}'

# Create student profile
curl -X POST http://localhost:5000/api/learning/profile \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "name": "John Doe",
    "assessment_results": {
      "visual_score": 8,
      "kinesthetic_score": 9,
      "skill_gpio": 0.6,
      "skill_timers": 0.3
    }
  }'

# Analyze code
curl -X POST http://localhost:5000/api/review/analyze \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -d '{
    "file_path": "main.c",
    "code_content": "#include <Arduino.h>\nvoid setup() { digitalWrite(13, HIGH); }"
  }'

# Get adaptive recommendations
curl -X GET http://localhost:5000/api/curriculum/recommendations \
  -H "Authorization: Bearer YOUR_TOKEN"
"""