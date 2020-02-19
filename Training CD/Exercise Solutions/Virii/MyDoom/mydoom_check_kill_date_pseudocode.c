// worm exits if this function returns true.
bool check_kill_date_mydoom_b(st_stored_kill_date)
{
    FILETIME ft_current_time,
             ft_stored_kill_date;
             
    GetSystemTimeAsFileTime(&ft_current_time);
    SystemTimeToFileTime(&st_stored_kill_date, &ft_stored_kill_date);
    
    if (ft_stored_kill_date.dwHighDateTime <= ft_current_time.dwHighDateTime)
        if (ft_stored_kill_date.dwLowDateTime > ft_current_time.dwLowDateTime)
            return TRUE;
        else
            return FALSE;
    else            
        return TRUE;
}


// worm exits if this function returns true.
bool check_kill_date_mydoom_a(st_stored_kill_date)
{
    FILETIME ft_current_time,
             ft_stored_kill_date;
             
    GetSystemTimeAsFileTime(&ft_current_time);
    SystemTimeToFileTime(&st_stored_kill_date, &ft_stored_kill_date);
    
    if (ft_stored_kill_date.dwHighDateTime <= ft_current_time.dwHighDateTime)
        if (ft_stored_kill_date.dwHighDateTime == ft_current_time.dwHighDateTime)
            return TRUE;
        else
            return FALSE;
    else
        return TRUE;
}


