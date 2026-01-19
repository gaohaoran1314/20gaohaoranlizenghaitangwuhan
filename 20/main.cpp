import std;
#include <pqxx/pqxx>


// 全局常量：封装数据库连接参数，避免硬编码
const std::string DB_CONN_STR = "dbname=student_sys user=postgres password=123456 host=localhost port=5432";
const int TABLE_WIDTH = 15; // 格式化输出列宽

// ====================== 领域层（实体类）======================
class Student {
private:
    std::string id;
    std::string name;
    std::string major;
public:
    Student(std::string id, std::string name, std::string major)
        : id(std::move(id)), name(std::move(name)), major(std::move(major)) {}

    std::string getId() const { return id; }
    std::string getName() const { return name; }
    std::string getMajor() const { return major; }
};

class Teacher {
private:
    std::string id;
    std::string name;
    std::string department;
public:
    Teacher(std::string id, std::string name, std::string department)
        : id(std::move(id)), name(std::move(name)), department(std::move(department)) {}

    std::string getId() const { return id; }
    std::string getName() const { return name; }
    std::string getDepartment() const { return department; }
};

class Course {
private:
    std::string id;
    std::string name;
    int credit;
    std::string teacherId;
public:
    Course(std::string id, std::string name, int credit, std::string teacherId)
        : id(std::move(id)), name(std::move(name)), credit(credit), teacherId(std::move(teacherId)) {}

    std::string getId() const { return id; }
    std::string getName() const { return name; }
    int getCredit() const { return credit; }
    std::string getTeacherId() const { return teacherId; }
};

class Score {
private:
    std::string studentId;
    std::string courseId;
    double score;
public:
    Score(std::string studentId, std::string courseId, double score)
        : studentId(std::move(studentId)), courseId(std::move(courseId)), score(score) {}

    std::string getStudentId() const { return studentId; }
    std::string getCourseId() const { return courseId; }
    double getScore() const { return score; }
    void setScore(double s) { score = s; }
};

// ====================== 工具类：数据库连接+输入处理 ======================
// 数据库连接工具：封装连接创建，避免重复代码
class DBUtil {
public:
    static pqxx::connection createConn() {
        try {
            pqxx::connection conn(DB_CONN_STR);
            if (conn.is_open()) {
                return conn;
            } else {
                throw std::runtime_error("数据库连接失败：连接未打开");
            }
        } catch (const pqxx::sql_error& e) {
            throw std::runtime_error("SQL错误：" + std::string(e.what()) + " | SQL：" + e.query());
        } catch (const std::exception& e) {
            throw std::runtime_error("数据库连接错误：" + std::string(e.what()));
        }
    }
};

// 输入处理工具：处理cin异常，避免死循环
class InputUtil {
public:
    // 清空输入缓冲区
    static void clearInput() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // 读取整数（带范围校验）
    static int readInt(int min, int max) {
        int num;
        while (true) {
            if (std::cin >> num && num >= min && num <= max) {
                clearInput();
                return num;
            } else {
                clearInput();
                std::cout << "输入无效，请输入" << min << "-" << max << "之间的整数：";
            }
        }
    }

    // 读取字符串（非空校验）
    static std::string readString(const std::string& tip) {
        std::string str;
        while (true) {
            std::cout << tip;
            std::cin >> str;
            if (!str.empty()) {
                return str;
            }
            std::cout << "输入不能为空！" << std::endl;
        }
    }

    // 读取浮点数（0-100范围，成绩专用）
    static double readScore() {
        double s;
        while (true) {
            std::cout << "输入成绩（0-100）：";
            if (std::cin >> s && s >= 0 && s <= 100) {
                clearInput();
                return s;
            } else {
                clearInput();
                std::cout << "成绩无效，请输入0-100的数字！" << std::endl;
            }
        }
    }
};

// ====================== 数据管理层（仓库层）======================
class StudentRepository {
private:
    pqxx::connection conn;
public:
    StudentRepository() : conn(DBUtil::createConn()) {}

    // 新增学生
    void addStudent(const Student& student) {
        try {
            pqxx::work txn(conn);
            txn.exec_params(
                "INSERT INTO students (id, name, major) VALUES ($1, $2, $3) ON CONFLICT (id) DO NOTHING",
                student.getId(), student.getName(), student.getMajor()
            );
            txn.commit();
            std::cout << "学生【" << student.getName() << "】新增成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("新增学生失败：" + std::string(e.what()));
        }
    }

    // 根据ID查询学生
    Student getStudentById(const std::string& id) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params("SELECT * FROM students WHERE id = $1", id);
            txn.commit();
            if (res.empty()) throw std::runtime_error("学生ID【" + id + "】不存在");
            return Student(
                res[0]["id"].as<std::string>(),
                res[0]["name"].as<std::string>(),
                res[0]["major"].as<std::string>()
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("查询学生失败：" + std::string(e.what()));
        }
    }

    // 查询所有学生
    std::vector<Student> getAllStudents() {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec("SELECT * FROM students ORDER BY id");
            txn.commit();
            std::vector<Student> students;
            for (const auto& row : res) {
                students.emplace_back(
                    row["id"].as<std::string>(),
                    row["name"].as<std::string>(),
                    row["major"].as<std::string>()
                );
            }
            return students;
        } catch (const std::exception& e) {
            throw std::runtime_error("查询所有学生失败：" + std::string(e.what()));
        }
    }

    // 删除学生
    void deleteStudent(const std::string& id) {
        try {
            // 先校验学生是否存在
            getStudentById(id);
            pqxx::work txn(conn);
            // 级联删除选课和成绩记录
            txn.exec_params("DELETE FROM scores WHERE student_id = $1", id);
            txn.exec_params("DELETE FROM enrollments WHERE student_id = $1", id);
            txn.exec_params("DELETE FROM students WHERE id = $1", id);
            txn.commit();
            std::cout << "学生ID【" << id << "】删除成功（含关联选课/成绩）！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("删除学生失败：" + std::string(e.what()));
        }
    }
};

class TeacherRepository {
private:
    pqxx::connection conn;
public:
    TeacherRepository() : conn(DBUtil::createConn()) {}

    // 新增教师
    void addTeacher(const Teacher& teacher) {
        try {
            pqxx::work txn(conn);
            txn.exec_params(
                "INSERT INTO teachers (id, name, department) VALUES ($1, $2, $3) ON CONFLICT (id) DO NOTHING",
                teacher.getId(), teacher.getName(), teacher.getDepartment()
            );
            txn.commit();
            std::cout << "教师【" << teacher.getName() << "】新增成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("新增教师失败：" + std::string(e.what()));
        }
    }

    // 根据ID查询教师
    Teacher getTeacherById(const std::string& id) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params("SELECT * FROM teachers WHERE id = $1", id);
            txn.commit();
            if (res.empty()) throw std::runtime_error("教师ID【" + id + "】不存在");
            return Teacher(
                res[0]["id"].as<std::string>(),
                res[0]["name"].as<std::string>(),
                res[0]["department"].as<std::string>()
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("查询教师失败：" + std::string(e.what()));
        }
    }
};

class CourseRepository {
private:
    pqxx::connection conn;
public:
    CourseRepository() : conn(DBUtil::createConn()) {}

    // 新增课程
    void addCourse(const Course& course) {
        try {
            pqxx::work txn(conn);
            txn.exec_params(
                "INSERT INTO courses (id, name, credit, teacher_id) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO NOTHING",
                course.getId(), course.getName(), course.getCredit(), course.getTeacherId()
            );
            txn.commit();
            std::cout << "课程【" << course.getName() << "】新增成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("新增课程失败：" + std::string(e.what()));
        }
    }

    // 根据ID查询课程
    Course getCourseById(const std::string& id) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params("SELECT * FROM courses WHERE id = $1", id);
            txn.commit();
            if (res.empty()) throw std::runtime_error("课程ID【" + id + "】不存在");
            return Course(
                res[0]["id"].as<std::string>(),
                res[0]["name"].as<std::string>(),
                res[0]["credit"].as<int>(),
                res[0]["teacher_id"].as<std::string>()
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("查询课程失败：" + std::string(e.what()));
        }
    }

    // 查询所有课程
    std::vector<Course> getAllCourses() {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec("SELECT * FROM courses ORDER BY id");
            txn.commit();
            std::vector<Course> courses;
            for (const auto& row : res) {
                courses.emplace_back(
                    row["id"].as<std::string>(),
                    row["name"].as<std::string>(),
                    row["credit"].as<int>(),
                    row["teacher_id"].as<std::string>()
                );
            }
            return courses;
        } catch (const std::exception& e) {
            throw std::runtime_error("查询所有课程失败：" + std::string(e.what()));
        }
    }

    // 删除课程
    void deleteCourse(const std::string& id) {
        try {
            getCourseById(id);
            pqxx::work txn(conn);
            txn.exec_params("DELETE FROM scores WHERE course_id = $1", id);
            txn.exec_params("DELETE FROM enrollments WHERE course_id = $1", id);
            txn.exec_params("DELETE FROM courses WHERE id = $1", id);
            txn.commit();
            std::cout << "课程ID【" << id << "】删除成功（含关联选课/成绩）！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("删除课程失败：" + std::string(e.what()));
        }
    }
};

class ScoreRepository {
private:
    pqxx::connection conn;
public:
    ScoreRepository() : conn(DBUtil::createConn()) {}

    // 录入/更新成绩
    void setScore(const Score& score) {
        try {
            pqxx::work txn(conn);
            // 先校验是否选课
            pqxx::result res = txn.exec_params(
                "SELECT * FROM enrollments WHERE student_id = $1 AND course_id = $2",
                score.getStudentId(), score.getCourseId()
            );
            if (res.empty()) throw std::runtime_error("学生未选该课程，无法录入成绩");
            // 存在则更新，不存在则插入
            txn.exec_params(
                "INSERT INTO scores (student_id, course_id, score) VALUES ($1, $2, $3) "
                "ON CONFLICT (student_id, course_id) DO UPDATE SET score = $3",
                score.getStudentId(), score.getCourseId(), score.getScore()
            );
            txn.commit();
            std::cout << "成绩录入/更新成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("成绩操作失败：" + std::string(e.what()));
        }
    }

    // 查询学生所有成绩
    std::vector<Score> getScoresByStudentId(const std::string& studentId) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params(
                "SELECT * FROM scores WHERE student_id = $1 ORDER BY course_id",
                studentId
            );
            txn.commit();
            std::vector<Score> scores;
            for (const auto& row : res) {
                scores.emplace_back(
                    row["student_id"].as<std::string>(),
                    row["course_id"].as<std::string>(),
                    row["score"].as<double>()
                );
            }
            if (scores.empty()) throw std::runtime_error("该学生暂无成绩记录");
            return scores;
        } catch (const std::exception& e) {
            throw std::runtime_error("查询成绩失败：" + std::string(e.what()));
        }
    }
};

class EnrollmentRepository {
private:
    pqxx::connection conn;
public:
    EnrollmentRepository() : conn(DBUtil::createConn()) {}

    // 选课（含重复校验）
    void enroll(const std::string& studentId, const std::string& courseId) {
        try {
            pqxx::work txn(conn);
            // 校验是否已选课
            pqxx::result res = txn.exec_params(
                "SELECT * FROM enrollments WHERE student_id = $1 AND course_id = $2",
                studentId, courseId
            );
            if (!res.empty()) throw std::runtime_error("已选该课程，无需重复选课");
            // 插入选课记录
            txn.exec_params(
                "INSERT INTO enrollments (student_id, course_id) VALUES ($1, $2)",
                studentId, courseId
            );
            txn.commit();
            std::cout << "学生【" << studentId << "】选课【" << courseId << "】成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("选课失败：" + std::string(e.what()));
        }
    }

    // 退课
    void dropCourse(const std::string& studentId, const std::string& courseId) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params(
                "SELECT * FROM enrollments WHERE student_id = $1 AND course_id = $2",
                studentId, courseId
            );
            if (res.empty()) throw std::runtime_error("未选该课程，无法退课");
            // 级联删除成绩
            txn.exec_params("DELETE FROM scores WHERE student_id = $1 AND course_id = $2", studentId, courseId);
            txn.exec_params("DELETE FROM enrollments WHERE student_id = $1 AND course_id = $2", studentId, courseId);
            txn.commit();
            std::cout << "学生【" << studentId << "】退课【" << courseId << "】成功！" << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("退课失败：" + std::string(e.what()));
        }
    }

    // 查询学生已选课程
    std::vector<Course> getEnrolledCourses(const std::string& studentId, CourseRepository& courseRepo) {
        try {
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params(
                "SELECT course_id FROM enrollments WHERE student_id = $1 ORDER BY course_id",
                studentId
            );
            txn.commit();
            std::vector<Course> courses;
            for (const auto& row : res) {
                std::string cid = row["course_id"].as<std::string>();
                courses.push_back(courseRepo.getCourseById(cid));
            }
            if (courses.empty()) throw std::runtime_error("该学生暂无选课记录");
            return courses;
        } catch (const std::exception& e) {
            throw std::runtime_error("查询选课记录失败：" + std::string(e.what()));
        }
    }
};

// ====================== 应用逻辑层（控制器）======================
class StudentController {
private:
    StudentRepository studentRepo;
public:
    void addStudent() {
        std::string id = InputUtil::readString("输入学生ID：");
        std::string name = InputUtil::readString("输入学生姓名：");
        std::string major = InputUtil::readString("输入学生专业：");
        try {
            studentRepo.addStudent(Student(id, name, major));
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void deleteStudent() {
        std::string id = InputUtil::readString("输入要删除的学生ID：");
        try {
            studentRepo.deleteStudent(id);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void listAllStudents() {
        try {
            auto students = studentRepo.getAllStudents();
            std::cout << "\n=== 所有学生列表 ===" << std::endl;
            std::cout << std::left << std::setw(TABLE_WIDTH) << "学生ID"
                      << std::setw(TABLE_WIDTH) << "姓名"
                      << std::setw(TABLE_WIDTH) << "专业" << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
            for (const auto& s : students) {
                std::cout << std::left << std::setw(TABLE_WIDTH) << s.getId()
                          << std::setw(TABLE_WIDTH) << s.getName()
                          << std::setw(TABLE_WIDTH) << s.getMajor() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
};

class CourseController {
private:
    CourseRepository courseRepo;
    TeacherRepository teacherRepo;
    StudentRepository studentRepo;
    EnrollmentRepository enrollRepo;
public:
    void addCourse() {
        std::string id = InputUtil::readString("输入课程ID：");
        std::string name = InputUtil::readString("输入课程名称：");
        std::cout << "输入课程学分：";
        int credit = InputUtil::readInt(1, 10);
        std::string tid = InputUtil::readString("输入授课教师ID：");
        try {
            // 校验教师是否存在
            teacherRepo.getTeacherById(tid);
            courseRepo.addCourse(Course(id, name, credit, tid));
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void deleteCourse() {
        std::string id = InputUtil::readString("输入要删除的课程ID：");
        try {
            courseRepo.deleteCourse(id);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void listAllCourses() {
        try {
            auto courses = courseRepo.getAllCourses();
            std::cout << "\n=== 所有课程列表 ===" << std::endl;
            std::cout << std::left << std::setw(TABLE_WIDTH) << "课程ID"
                      << std::setw(TABLE_WIDTH) << "课程名称"
                      << std::setw(TABLE_WIDTH) << "学分" << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
            for (const auto& c : courses) {
                std::cout << std::left << std::setw(TABLE_WIDTH) << c.getId()
                          << std::setw(TABLE_WIDTH) << c.getName()
                          << std::setw(TABLE_WIDTH) << c.getCredit() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void enrollStudent() {
        std::string sid = InputUtil::readString("输入学生ID：");
        std::string cid = InputUtil::readString("输入课程ID：");
        try {
            // 先校验学生和课程是否存在
            studentRepo.getStudentById(sid);
            courseRepo.getCourseById(cid);
            enrollRepo.enroll(sid, cid);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void dropStudentCourse() {
        std::string sid = InputUtil::readString("输入学生ID：");
        std::string cid = InputUtil::readString("输入课程ID：");
        try {
            enrollRepo.dropCourse(sid, cid);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void listStudentCourses() {
        std::string sid = InputUtil::readString("输入学生ID：");
        try {
            studentRepo.getStudentById(sid);
            auto courses = enrollRepo.getEnrolledCourses(sid, courseRepo);
            std::cout << "\n=== 学生【" << sid << "】已选课程 ===" << std::endl;
            std::cout << std::left << std::setw(TABLE_WIDTH) << "课程ID"
                      << std::setw(TABLE_WIDTH) << "课程名称"
                      << std::setw(TABLE_WIDTH) << "学分" << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
            for (const auto& c : courses) {
                std::cout << std::left << std::setw(TABLE_WIDTH) << c.getId()
                          << std::setw(TABLE_WIDTH) << c.getName()
                          << std::setw(TABLE_WIDTH) << c.getCredit() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
};

class TeacherController {
private:
    TeacherRepository teacherRepo;
public:
    void addTeacher() {
        std::string id = InputUtil::readString("输入教师ID：");
        std::string name = InputUtil::readString("输入教师姓名：");
        std::string dept = InputUtil::readString("输入教师所属院系：");
        try {
            teacherRepo.addTeacher(Teacher(id, name, dept));
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
};

class ScoreController {
private:
    ScoreRepository scoreRepo;
    StudentRepository studentRepo;
    CourseRepository courseRepo;
public:
    void inputScore() {
        std::string sid = InputUtil::readString("输入学生ID：");
        std::string cid = InputUtil::readString("输入课程ID：");
        double score = InputUtil::readScore();
        try {
            studentRepo.getStudentById(sid);
            courseRepo.getCourseById(cid);
            scoreRepo.setScore(Score(sid, cid, score));
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void queryStudentScore() {
        std::string sid = InputUtil::readString("输入学生ID：");
        try {
            auto student = studentRepo.getStudentById(sid);
            auto scores = scoreRepo.getScoresByStudentId(sid);
            std::cout << "\n=== 学生【" << student.getName() << "(" << sid << ")】成绩列表 ===" << std::endl;
            std::cout << std::left << std::setw(TABLE_WIDTH) << "课程ID"
                      << std::setw(TABLE_WIDTH) << "课程名称"
                      << std::setw(TABLE_WIDTH) << "成绩" << std::endl;
            std::cout << "---------------------------------------------" << std::endl;
            double sum = 0.0;
            for (const auto& s : scores) {
                auto course = courseRepo.getCourseById(s.getCourseId());
                sum += s.getScore();
                std::cout << std::left << std::setw(TABLE_WIDTH) << s.getCourseId()
                          << std::setw(TABLE_WIDTH) << course.getName()
                          << std::setw(TABLE_WIDTH) << std::fixed << std::setprecision(1) << s.getScore() << std::endl;
            }
            std::cout << "---------------------------------------------" << std::endl;
            std::cout << "平均分：" << std::fixed << std::setprecision(1) << sum / scores.size() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
};

// ====================== 表现层（终端交互）======================
class TerminalUI {
private:
    StudentController studentCtrl;
    CourseController courseCtrl;
    TeacherController teacherCtrl;
    ScoreController scoreCtrl;

    // 打印主菜单
    void printMainMenu() {
        std::cout << "\n=====================================" << std::endl;
        std::cout << "=========== 学生选课管理系统 ===========" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "1. 学生管理（增/删/查）" << std::endl;
        std::cout << "2. 教师管理（新增）" << std::endl;
        std::cout << "3. 课程管理（增/删/查）" << std::endl;
        std::cout << "4. 选课/退课管理" << std::endl;
        std::cout << "5. 成绩管理（录入/查询）" << std::endl;
        std::cout << "0. 退出系统" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "请输入功能编号：";
    }

    // 学生管理子菜单
    void studentMenu() {
        int choice;
        do {
            std::cout << "\n----- 学生管理子菜单 -----" << std::endl;
            std::cout << "1. 新增学生" << std::endl;
            std::cout << "2. 删除学生" << std::endl;
            std::cout << "3. 查看所有学生" << std::endl;
            std::cout << "0. 返回主菜单" << std::endl;
            std::cout << "请输入选择：";
            choice = InputUtil::readInt(0, 3);
            switch (choice) {
                case 1: studentCtrl.addStudent(); break;
                case 2: studentCtrl.deleteStudent(); break;
                case 3: studentCtrl.listAllStudents(); break;
                case 0: std::cout << "返回主菜单..." << std::endl; break;
            }
        } while (choice != 0);
    }

    // 教师管理子菜单
    void teacherMenu() {
        int choice;
        do {
            std::cout << "\n----- 教师管理子菜单 -----" << std::endl;
            std::cout << "1. 新增教师" << std::endl;
            std::cout << "0. 返回主菜单" << std::endl;
            std::cout << "请输入选择：";
            choice = InputUtil::readInt(0, 1);
            switch (choice) {
                case 1: teacherCtrl.addTeacher(); break;
                case 0: std::cout << "返回主菜单..." << std::endl; break;
            }
        } while (choice != 0);
    }

    // 课程管理子菜单
    void courseMenu() {
        int choice;
        do {
            std::cout << "\n----- 课程管理子菜单 -----" << std::endl;
            std::cout << "1. 新增课程" << std::endl;
            std::cout << "2. 删除课程" << std::endl;
            std::cout << "3. 查看所有课程" << std::endl;
            std::cout << "0. 返回主菜单" << std::endl;
            std::cout << "请输入选择：";
            choice = InputUtil::readInt(0, 3);
            switch (choice) {
                case 1: courseCtrl.addCourse(); break;
                case 2: courseCtrl.deleteCourse(); break;
                case 3: courseCtrl.listAllCourses(); break;
                case 0: std::cout << "返回主菜单..." << std::endl; break;
            }
        } while (choice != 0);
    }

    // 选课退课子菜单
    void enrollMenu() {
        int choice;
        do {
            std::cout << "\n----- 选课/退课管理子菜单 -----" << std::endl;
            std::cout << "1. 学生选课" << std::endl;
            std::cout << "2. 学生退课" << std::endl;
            std::cout << "3. 查看学生已选课程" << std::endl;
            std::cout << "0. 返回主菜单" << std::endl;
            std::cout << "请输入选择：";
            choice = InputUtil::readInt(0, 3);
            switch (choice) {
                case 1: courseCtrl.enrollStudent(); break;
                case 2: courseCtrl.dropStudentCourse(); break;
                case 3: courseCtrl.listStudentCourses(); break;
                case 0: std::cout << "返回主菜单..." << std::endl; break;
            }
        } while (choice != 0);
    }

    // 成绩管理子菜单
    void scoreMenu() {
        int choice;
        do {
            std::cout << "\n----- 成绩管理子菜单 -----" << std::endl;
            std::cout << "1. 录入/更新成绩" << std::endl;
            std::cout << "2. 查询学生成绩（含平均分）" << std::endl;
            std::cout << "0. 返回主菜单" << std::endl;
            std::cout << "请输入选择：";
            choice = InputUtil::readInt(0, 2);
            switch (choice) {
                case 1: scoreCtrl.inputScore(); break;
                case 2: scoreCtrl.queryStudentScore(); break;
                case 0: std::cout << "返回主菜单..." << std::endl; break;
            }
        } while (choice != 0);
    }

public:
    void run() {
        try {
            std::cout << "系统启动中...数据库连接成功！" << std::endl;
            int choice;
            do {
                printMainMenu();
                choice = InputUtil::readInt(0, 5);
                switch (choice) {
                    case 1: studentMenu(); break;
                    case 2: teacherMenu(); break;
                    case 3: courseMenu(); break;
                    case 4: enrollMenu(); break;
                    case 5: scoreMenu(); break;
                    case 0: std::cout << "\n感谢使用学生选课管理系统，再见！" << std::endl; break;
                }
            } while (choice != 0);
        } catch (const std::exception& e) {
            std::cerr << "\n系统启动失败：" << e.what() << std::endl;
            std::cerr << "请检查数据库连接或表结构是否正确！" << std::endl;
        }
    }
};

// ====================== 主函数 ======================
int main() {

    TerminalUI ui;
    ui.run();
    return 0;
}
