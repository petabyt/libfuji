//#define PTP_MUTEX_CHECK
#ifdef BUILD_TYPE_RELEASE
	#ifdef PTP_MUTEX_CHECK
		#error "Dev forgot to uncomment PTP_MUTEX_CHECK"
	#endif
#endif
#ifdef PTP_MUTEX_CHECK
struct PtpLockToken { struct PtpRuntime *r; };

__attribute__((unused))
static inline struct PtpLockToken _ptp_mutex_lock_internal(struct PtpRuntime *r) {
	ptp_mutex_lock(r);
	return (struct PtpLockToken){ .r = r };
}

__attribute__((unused))
static inline void _ptp_mutex_unlock_cleanup(struct PtpLockToken *t) {
	ptp_panic("Missing ptp_mutex_unlock()");
	if (t && t->r)
		ptp_mutex_unlock(t->r);
}

#define _PTP_CONCAT(a, b) a##b
#define _PTP_CONCAT2(a, b) _PTP_CONCAT(a, b)

#define ptp_mutex_lock(r) \
	struct PtpLockToken _PTP_CONCAT2(temp, __LINE__) __attribute__((cleanup(_ptp_mutex_unlock_cleanup))) = _ptp_mutex_lock_internal(r)
#endif
