#include <asl/Vec3.h>
#include <asl/Vec4.h>
#include <asl/Matrix4.h>
#include <asl/Matrix3.h>
#include <asl/Quaternion.h>
#include <asl/Uuid.h>
#include <asl/Array2.h>
#include <asl/Matrix.h>
#include <asl/StreamBuffer.h>
#include <asl/Http.h>
#include <stdio.h>
#include <asl/testing.h>

using namespace asl;

#define EPS 1e-6
#define EPSf 1e-5f


ASL_TEST(Vec3)
{
	Vec3 a(1, 2.5f, 3), b(1, 0, 0);
	a += Vec3::zeros();
	ASL_APPROX(a*b, 1.0f, EPS);

	Vec3d a2 = a, b2 = b;
	Vec3 a3 = a2.with<float>();
	ASL_APPROX(a2 + b2, Vec3d(2, 2.5, 3), EPS);
}

ASL_TEST(Matrix4)
{
	double angle = deg2rad(90);
	ASL_APPROX(rad2deg(angle), 90.0, 0.01);

	Matrix4d m1 = Matrix4d::rotate(Vec3d(1, 0, 0), PI / 2);
	Matrix4d m2 = Matrix4d::rotateX(PI / 2);
	Vec3d a(0, 1, 0);
	ASL_APPROX(m1 * a, Vec3d(0, 0, 1), EPS);
	ASL_APPROX(m2 * a, Vec3d(0, 0, 1), EPS);

	Quaterniond q1 = Quaterniond::fromAxisAngle(Vec3d(1, 0.5f, -1.25f), 0.25f);
	Matrix4d mrot = Matrix4d::rotate(Vec3d(1, 0.5f, -1.25f), 0.25f);
	Quaterniond q2 = mrot.rotation();

	Vec3d rv1 = mrot.axisAngle();
	Vec3d rv2 = mrot.rotation().axisAngle();
	ASL_APPROX(rv1, rv2, EPS);
	Matrix4d mrot2 = Matrix4d::rotate(rv1);
	ASL_APPROX(mrot, mrot2, EPS);

	ASL_APPROX(q1, q2, EPS);
	ASL_APPROX(q1.matrix(), mrot, EPS);
	ASL_APPROX(q1.matrix(), q2.matrix(), EPS);

	for (double x = -1; x <= 1; x+=0.5)
		for (double y = -1; y <= 1; y += 0.5)
			for (double z = -1; z <= 1; z += 0.5)
				for (double a = -2*PI; a <= 2*PI; a += PI / 8)
				{
					if (x == 0 && y == 0 && z == 0)
						continue;
					Quaterniond q1 = Quaterniond::fromAxisAngle(Vec3d(x, y, z), a);
					Matrix4d m = q1.matrix();
					Quaterniond q2 = m.rotation();
					ASL_APPROX(q1, q2, EPS);
					ASL_APPROX(q1.matrix(), q2.matrix(), EPS);
				}

	m1 = Matrix4d::translate(1, 2, 3) * mrot;
	Matrix4d mi = m1.inverse();
	ASL_APPROX(mi * m1, Matrix4d::identity(), EPS);
	ASL_APPROX(mi.inverse(), m1, EPS);

	m1(3, 0) = 0.5;
	m1(3, 1) = -0.1;
	m1(3, 2) = 1.5;

	Vec3d v3(1, 2, 3);

	ASL_APPROX((m1 ^ v3), (m1 * Vec4d(v3, 1)).h2c(), EPS);

	Matrix3 h = Matrix3::translate(1.0f, -2.0f) * Matrix3::rotate(0.5f);
	h(2, 0) = 0.2f;
	h(2, 1) = -0.15f;
	Vec2 v2(1, 3);
	ASL_APPROX((h ^ v2), (h * Vec3(v2, 1)).h2c(), EPS);
}

ASL_TEST(Uuid)
{
	Uuid u1 ("93efe45f-97b8-487f-a1a1-a08838ca3598");
	Uuid u2 ("93efe45F-97b8-487F-A1a1-a08838Ca3598");
	Uuid u3 = Uuid::generate();
	ASL_ASSERT(u1 == u2);
	ASL_ASSERT(*u1 == "93efe45f-97b8-487f-a1a1-a08838ca3598");
	ASL_ASSERT(u3 != u2);
}

ASL_TEST(StreamBuffer)
{
	StreamBuffer b;
	b.setEndian(ENDIAN_LITTLE);
	b << 'a' << 4 << 3.5 << true;

	ASL_ASSERT(b.length() == 14);
	ASL_ASSERT(b[0] == 'a' && b[1] == 0x04 && b[2] == 0 && b[3] == 0 && b[4] == 0)

	StreamBufferReader c(b.ptr(), b.length());
	signed char a;
	int x;
	double y;
	bool f;
	c >> a >> x >> y >> f;

	ASL_ASSERT(a == 'a' && x == 4 && y == 3.5 && f == true);

	StreamBuffer b2;
	b2.setEndian(ENDIAN_BIG);
	b2 << 'a' << 4 << 3.5;

	a = ' ';
	x = 0;
	y = 0;

	ASL_ASSERT(b2.length() == 13);
	ASL_ASSERT(b2[0] == 'a' && b2[1] == 0 && b2[2] == 0 && b2[3] == 0 && b2[4] == 4)

	StreamBufferReader c2(b2.ptr(), b2.length(), ENDIAN_BIG);
	c2 >> a >> x >> y;

	ASL_ASSERT(a == 'a' && x == 4 && y == 3.5);
}

ASL_TEST(Array2)
{
	Array2<int> a(2, 3);
	for (int i = 0; i < a.rows(); i++)
		for (int j = 0; j < a.cols(); j++)
			a(i, j) = i * 10 + j;

	ASL_ASSERT(a(1, 2) == 12);

#ifdef ASL_HAVE_INITLIST
	Array2<int> a2(2, 3, {
		1, 2, 3,
		4, 5, 6
	});

	Array2<int> a3 = {
		{ 1, 2, 3 },
		{ 4, 5, 6 }
	};

	ASL_ASSERT(a2 == a3);
#endif

	Array2<int> a4 = a.slice(0, 2, 1, 2);

	ASL_ASSERT(a4 == Array2<int>(2, 1, array(1, 11)));
}

ASL_TEST(Matrix)
{
	Matrix A(2, 2, array<float>(
		1, -1,
		2, 3
	));

	Matrix B = A.inverse() * A;

	ASL_CHECK((B - Matrix::identity(2)).norm(), <, 1e-6f);

#ifdef ASL_HAVE_INITLIST
	Matrix C = {
		{ 1, -1 },
		{ 2, 3 }
	};

	ASL_ASSERT(C == A);

	C = { { 1, 2 } };

	ASL_ASSERT(C.rows() == 1 && C.cols() == 2);
	ASL_ASSERT(C(0, 0) == 1 && C(0, 1) == 2);

	C = { 3, 5 };
	ASL_ASSERT(C.rows() == 2 && C.cols() == 1);
	ASL_ASSERT(C(0, 0) == 3 && C(1, 0) == 5);
	ASL_ASSERT(C[0] == 3 && C[1] == 5);

#endif
}

ASL_TEST(URL)
{
	String p = Url::params(Dic<>("x", "a b")("y", "3"));
	ASL_ASSERT(p == "x=a%20b&y=3");
	Dic<> q = Url::parseQuery(p);
	ASL_ASSERT(q["x"] == "a b" && q["y"] == "3");
	Url u("http://w.org/path");
	ASL_ASSERT(u.host == "w.org");
	ASL_ASSERT(u.path == "/path");
	ASL_ASSERT(Url::encode("a\t b") == "a%09%20b");
	ASL_ASSERT(Url::decode("a%09%20b") == "a\t b");
}
